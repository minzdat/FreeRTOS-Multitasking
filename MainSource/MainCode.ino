#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <DHT11.h>
#include <HCSR04.h>
#include <Buzzer.h>
#include <Bonezegei_LCD1602_I2C.h>

// Khai báo chân
#define DHT11_PIN 15
#define TRIGGER_PIN 16
#define ECHO_PIN 17
const int buttonPin1 = 4; 
const int buttonPin2 = 5; 
const int buttonPinSemaphore = 19;
const int ledPin =  32;    
const int ledPinSemaphore =  33;   

// Tạo đối tượng DHT11, Buzzer và LCD
DHT11 dht11(DHT11_PIN);
Buzzer buzzer(13, 15);
Bonezegei_LCD1602_I2C lcd(0x27);

// Biến toàn cục để lưu giá trị cảm biến
int temperature = 0;
int humidity = 0;
double distance = 0;
// Biến lưu trữ trạng thái nút nhấn và Led
int buttonState1 = 0;
int buttonState2 = 0;
bool ledState = false;
// Biến lưu trữ trạng thái blink Led
int countBlinkLed = 0;

// Semaphore
SemaphoreHandle_t xSerialSemaphore;
// Software Timer
TimerHandle_t lcdTimer;

void semaphoreButtonTask(void *pvParameters __attribute__((unused)) );
void semaphoreLedTask(void *pvParameters __attribute__((unused)) );
void sensorTask(void *pvParameters);
void buzzerTask(void *pvParameters);
void buttonLedTask(void *pvParameters);
void lcdTask(void *pvParameters);
void lcdTimerCallback(TimerHandle_t xTimer);

void setup() {
  Serial.begin(115200);
  HCSR04.begin(TRIGGER_PIN, ECHO_PIN);

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPinSemaphore, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPinSemaphore, OUTPUT);

  if ( xSerialSemaphore == NULL ) 
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) ); 
  }
  xTaskCreatePinnedToCore(sensorTask, "Sensor Task", 2048, NULL, 4, NULL, 0);   
  xTaskCreatePinnedToCore(lcdTask, "LCD Task", 2048, NULL, 4, NULL, 0);       
  xTaskCreatePinnedToCore(buzzerTask, "Buzzer Task", 2048, NULL, 3, NULL, 0);              

  xTaskCreatePinnedToCore(buttonLedTask, "Button LED Task", 2048, NULL, 2, NULL, 1);       
  xTaskCreatePinnedToCore(semaphoreButtonTask, "Button Semaphore Task", 2048, NULL, 1, NULL, 1); 
  xTaskCreatePinnedToCore(semaphoreLedTask, "Semaphore LED Task", 2048, NULL, 1, NULL, 1);  
}

void loop() {
  // Empty. Everything is handled by tasks.
}

// Hàm callback của Software Timer
void lcdTimerCallback(TimerHandle_t xTimer) {

  // Đọc thành công, cập nhật hiển thị LCD
  lcd.clear();
  lcd.setPosition(0, 0);
  lcd.print("Temp:");
  lcd.print(String(temperature).c_str()); 
  lcd.print("C");

  lcd.setPosition(0, 1);
  lcd.print("Humi:");
  lcd.print(String(humidity).c_str()); 
  lcd.print("%");

  lcd.setPosition(9, 0);
  lcd.print("Dist:");
  lcd.setPosition(9, 1);
  lcd.print(String(distance).c_str()); 
  lcd.print("cm");

}

void semaphoreButtonTask( void *pvParameters __attribute__((unused)) ) {
  const TickType_t semaphoreDelay = pdMS_TO_TICKS(500);
  int buttonPressCount = 0;
  bool lastButtonState = false;
  bool statusCount = false;

  for (;;) {
    if(digitalRead(buttonPinSemaphore) == HIGH){
      unsigned long startTime = millis();    
      while (millis() - startTime < 2000) {
        bool buttonState = digitalRead(buttonPinSemaphore);
        if (buttonState == HIGH && lastButtonState == LOW) {
          if (xSemaphoreTake(xSerialSemaphore, (TickType_t) 5) == pdTRUE) {
            statusCount = true;
            // Serial.println("Task semaphoreButtonTask success");
          }
          if (statusCount == true){
            if (buttonPressCount < 5){
              buttonPressCount++; 
              // Serial.println("Button Semaphore pressed ");
            }
          }
        }
        lastButtonState = buttonState;
      }
      countBlinkLed = buttonPressCount;
      // Serial.print("Button Semaphore pressed ");
      // Serial.print(countBlinkLed);
      // Serial.println(" time(s)");
      statusCount = false;
      buttonPressCount = 0;
      xSemaphoreGive( xSerialSemaphore ); 
    }
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void semaphoreLedTask( void *pvParameters __attribute__((unused)) ) {
  const TickType_t blinkDelay = pdMS_TO_TICKS(500); 
  for (;;) {
    if (xSemaphoreTake(xSerialSemaphore, (TickType_t) 5) == pdTRUE) {
      // Serial.println("Task semaphoreLedTask success");
      for (int i = 0; i < countBlinkLed; i++) {
        for (int i = 0; i < 3; i++) {
          digitalWrite(ledPinSemaphore, HIGH);
          vTaskDelay(blinkDelay);
          digitalWrite(ledPinSemaphore, LOW);
          vTaskDelay(blinkDelay);
        }
        vTaskDelay(blinkDelay);
      }
      countBlinkLed = 0;
      xSemaphoreGive( xSerialSemaphore ); 
    }
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void sensorTask(void *pvParameters) {
  for (;;) {
    // Serial.println("Task sensorTask success");
    int result = dht11.readTemperatureHumidity(temperature, humidity);
    if (result == 0) {
      // Successfully read temperature and humidity
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C\tHumidity: ");
      Serial.print(humidity);
      Serial.println(" %");
    } else {
      Serial.println(DHT11::getErrorString(result));
    }
    double *measureDistance = HCSR04.measureDistanceCm();      
    if (measureDistance != nullptr) {
      distance = measureDistance[0];
      Serial.print("Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
    } else {
      Serial.println("Failed to measure distance");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void buzzerTask(void *pvParameters) {
  for (;;) {
    buzzer.begin(100);
    // Serial.println("buzzerTask Success");

    buzzer.sound(NOTE_E7, 80);
    buzzer.sound(NOTE_E7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_E7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_C7, 80);
    buzzer.sound(NOTE_E7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_G7, 80);
    buzzer.sound(0, 240);
    buzzer.sound(NOTE_G6, 80);
    buzzer.sound(0, 240);
    buzzer.sound(NOTE_C7, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_G6, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_E6, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_A6, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_B6, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_AS6, 80);
    buzzer.sound(NOTE_A6, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_G6, 100);
    buzzer.sound(NOTE_E7, 100);
    buzzer.sound(NOTE_G7, 100);
    buzzer.sound(NOTE_A7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_F7, 80);
    buzzer.sound(NOTE_G7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_E7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_C7, 80);
    buzzer.sound(NOTE_D7, 80);
    buzzer.sound(NOTE_B6, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_C7, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_G6, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_E6, 80);
    buzzer.sound(0, 160);
    buzzer.sound(NOTE_A6, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_B6, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_AS6, 80);
    buzzer.sound(NOTE_A6, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_G6, 100);
    buzzer.sound(NOTE_E7, 100);
    buzzer.sound(NOTE_G7, 100);
    buzzer.sound(NOTE_A7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_F7, 80);
    buzzer.sound(NOTE_G7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_E7, 80);
    buzzer.sound(0, 80);
    buzzer.sound(NOTE_C7, 80);
    buzzer.sound(NOTE_D7, 80);
    buzzer.sound(NOTE_B6, 80);
    buzzer.sound(0, 160);

    buzzer.end(2000);
    vTaskDelay(pdMS_TO_TICKS(500)); 
  }
}

void buttonLedTask(void *pvParameters) {
  for (;;) {
    // Serial.println("Task buttonLedTask success");
    buttonState1 = digitalRead(buttonPin1);
    buttonState2 = digitalRead(buttonPin2);
    if (buttonState1 == HIGH) {
      ledState = true;
    }
    if (buttonState2 == HIGH) {
      ledState = false;
    }
    digitalWrite(ledPin, ledState ? HIGH : LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void lcdTask(void *pvParameters) {
  // Khởi tạo LCD
  lcd.begin();

  // Tạo Software Timer để cập nhật hiển thị LCD mỗi giây
  lcdTimer = xTimerCreate("LCDTimer", pdMS_TO_TICKS(1000), pdTRUE, NULL, lcdTimerCallback);
  if (lcdTimer != NULL) {
    xTimerStart(lcdTimer, 0);
  } else {
    Serial.println("Failed to create LCD Timer");
    vTaskDelete(NULL);
  }

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
