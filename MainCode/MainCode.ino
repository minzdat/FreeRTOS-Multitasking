// Thư viện cần thiết
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <stdbool.h>
#include "AsyncUDP.h"
#include "PCF8574.h"

// Thư viện freeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/stream_buffer.h"

//____________________TASK Mailbox - TASK Stream Buffers____________________________
// Define all pins and settings from both files
#define LED2_PIN 19
#define BUTTON_SEMA 4
#define CS_PIN 5
#define DATA_PIN 23
#define CLK_PIN 18

#define LED_R_PIN 21 
#define LED_G_PIN 16 
#define LED_B_PIN 17 

#define WS2812_PIN 13 
#define LED_R 12
#define LED_G 14
#define LED_B 27
#define BUTTON1_PIN 15
#define BUTTON2_PIN 2
#define BUTTON3_PIN 0
#define BUTTON4_PIN 22

// Global variables and semaphores
SemaphoreHandle_t countingSemaLed5;
SemaphoreHandle_t ws2812Mutex;
QueueHandle_t task2Queue;
QueueHandle_t task3Queue;
StreamBufferHandle_t xStreamBuffer;
TaskHandle_t task1ReceiverHandle = NULL;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
Adafruit_NeoPixel ws2812(4, WS2812_PIN, NEO_GRB + NEO_KHZ800);
// Định nghĩa các trạng thái của LED RGB thông thường và LED WS2812
enum WS2812State { WS2812_RED, WS2812_GREEN, WS2812_BLUE, WS2812_WHITE, WS2812_OFF };
WS2812State ws2812State = WS2812_OFF;
//TASK Mailbox-TASK Stream Buffers

//____________________TASK INTERRUPT____________________________
#define LED_SEMA_PIN 18
#define LED_INTERRUPT_SEMA_PIN 19
#define LED_QUEUE_PIN 32
#define LED_INTERRUPT_QUEUE_PIN 33

#define BUTTON_SEMA 15
#define BUTTON_SEMA_INTERRUPT 2
#define BUTTON_QUEUE 16
#define BUTTON_QUEUE_INTERRUPT 17

// SemaphoreHandle_t countingSemaLed5;
QueueHandle_t ledQueue;
SemaphoreHandle_t semaInterrupt;
SemaphoreHandle_t queueInterrupt;

bool ledInterruptSemaState = LOW;
bool ledInterruptQueueState = LOW;
//TASK INTERRUPT


//____________________TASK QUEUE - TASK NOFITICATION____________________________
// Định nghĩa các chân cho task queue và task nofitication
#define LED_CIR_PIN       22       // Chân kết nối led ws2812 board tròn
#define NUMCIR            8        // Số lượng LED
#define BUTTON_CIR_PIN    4        // Chân kết nối nút nhấn led tròn
#define LED_ST_PIN        23       // Chân kết nối led ws2812 board thẳng
#define NUMST             8        // Số lượng LED
#define BUTTON_ST         2        // Chân kết nối nút nhấn led thẳng

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUMCIR, LED_CIR_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUMST, LED_ST_PIN, NEO_GRB + NEO_KHZ800);

TaskHandle_t TaskHandle_1 = NULL;
TaskHandle_t TaskHandle_2 = NULL;
TaskHandle_t xHandleLEDSTTask = NULL;
QueueHandle_t MessageQueue;
volatile int mode = 0; 

// Định nghĩa các lệnh LEDCIR
typedef enum {
    CASE1,
    CASE2,
    CASE3
} LedCommand_t;

//TASK QUEUE - TASK NOFITICATION

// ID và password WiFi
const char* ssid = "Minz";
const char* password = "12345678";

// Thiết lập NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

// Thiết lập TM1637
#define CLK 16
#define DIO 17
TM1637Display tm1637Display(CLK, DIO);

// Thiết lập OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_SDA 18
#define OLED_SCL 19
Adafruit_SSD1306 oledDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LED PWM pin
#define LED_PIN 25

// Semaphore
SemaphoreHandle_t xSerialSemaphore;

// Button pins
#define BTN_TM1637 12
#define BTN_OLED 14

// Biến toàn cục thời gian
int hoursOLED = 0;
int minutesOLED = 0;
int secondsOLED = 0;
int minutesTM1637 = 0;
int secondsTM1637 = 0;
int getTime = 0;

// Thiết lập UDP nhận dữ liệu 
AsyncUDP udp;
int arcValue = 0;
int btn7Value = 0;
int btn8Value = 0;

void TM1637DisplayTask(void *pvParameters);
void OLEDDisplayTask(void *pvParameters);
void UDPListenerTask(void *pvParameters); 
void LEDBrightnessTask(void *pvParameters);

void SetUpWIFI();
void SetUpOLED();
void updateDisplayTime(int* hours, int* minutes, int* seconds);
void handleUdpPacket(AsyncUDPPacket packet);

//____________________TASK QUEUE - TASK NOFITICATION____________________________

void BUTTONCIRTask(void *pvParameters);
void ButtonSTTask(void *pvParameters);
void handleMode1();
void handleMode2();
void handleMode3();
void case1Pattern();
void case2Pattern();
void case3Pattern();
void LEDCIRTask(void *pvParameters);
void LEDSTTask(void *pvParameters);

//TASK QUEUE - TASK NOFITICATION

//____________________TASK INTERRUPT____________________________
void IRAM_ATTR HandleSemaInterrupt();
void IRAM_ATTR HandleQueueInterrupt();
void ReadSemaButtonTask(void *pvParameters);
void WriteSemaLedTask(void *pvParameters);
void HandleSemaInterruptTask(void *pvParameters);
void ReadQueueButtonTask(void *pvParameters);
void HandleQueueTask(void *pvParameters);
void HandleQueueInterruptTask(void *pvParameters);
//TASK INTERRUPT

//____________________TASK Mailbox - TASK Stream Buffers____________________________
void setWS2812Color(uint8_t r, uint8_t g, uint8_t b) ;
void vTimerCallback(TimerHandle_t xTimer);
void softtimer(void *pvParameters);
void readSemaButton(void *pvParameters);
void writeSemaLed(void *pvParameters);
void task1Sender(void *pvParameters);
void task1Receiver(void *pvParameters);
void task2(void* pvParameters);
void taskCheckButton2(void *pvParameters);
void task3(void* pvParameters);
void taskCheckButton3(void *pvParameters);
void taskCheckButton4(void *pvParameters);
void task4(void *pvParameters);
//TASK Mailbox - TASK Stream Buffers

void setup() {
  Serial.begin(115200);

  //____________________TASK Mailbox - TASK Stream Buffers____________________________
  Wire.begin();
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON_SEMA, INPUT_PULLUP);
  pinMode(LED_R_PIN, OUTPUT); 
  pinMode(LED_G_PIN, OUTPUT); 
  pinMode(LED_B_PIN, OUTPUT); 
  pinMode(WS2812_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT); 
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT); 
  pinMode(BUTTON1_PIN, INPUT_PULLUP); 
  pinMode(BUTTON2_PIN, INPUT_PULLUP); 
  pinMode(BUTTON3_PIN, INPUT_PULLUP); 
  pinMode(BUTTON4_PIN, INPUT_PULLUP);

  countingSemaLed5 = xSemaphoreCreateCounting(4, 0);
  ws2812Mutex = xSemaphoreCreateMutex();
  task2Queue = xQueueCreate(2, sizeof(uint8_t)); 
  task3Queue = xQueueCreate(2, sizeof(uint8_t));
  xStreamBuffer = xStreamBufferCreate(1028, 1); 

  P.begin();
  P.displayClear();
  //TASK Mailbox - TASK Stream Buffers

  //____________________TASK QUEUE - TASK NOFITICATION____________________________
  strip1.begin();
  strip1.show(); 
  strip2.begin();
  strip2.show(); 

  MessageQueue = xQueueCreate(10, sizeof(LedCommand_t)); //tạo hàng đợi với số phần tử tối đa là 10
  if (MessageQueue == NULL) {
      Serial.println("Failed to create message queue");
      while (true);
  }

  //TASK QUEUE - TASK NOFITICATION

  //____________________TASK INTERRUPT____________________________
  pinMode(BUTTON_SEMA, INPUT_PULLUP);
  pinMode(BUTTON_QUEUE, INPUT_PULLUP);
  pinMode(BUTTON_SEMA_INTERRUPT, INPUT_PULLUP);
  pinMode(BUTTON_QUEUE_INTERRUPT, INPUT_PULLUP);

  pinMode(LED_SEMA_PIN, OUTPUT);
  pinMode(LED_INTERRUPT_SEMA_PIN, OUTPUT);
  pinMode(LED_QUEUE_PIN, OUTPUT);
  pinMode(LED_INTERRUPT_QUEUE_PIN, OUTPUT);

  digitalWrite(LED_INTERRUPT_SEMA_PIN, ledInterruptSemaState); // Thiết lập trạng thái ban đầu cho LED_INTERRUPT_SEMA_PIN
  digitalWrite(LED_INTERRUPT_QUEUE_PIN, ledInterruptQueueState); // Thiết lập trạng thái ban đầu cho LED_INTERRUPT_QUEUE_PIN

  attachInterrupt(digitalPinToInterrupt(BUTTON_SEMA_INTERRUPT), HandleSemaInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_QUEUE_INTERRUPT), HandleQueueInterrupt, FALLING);

  countingSemaLed5 = xSemaphoreCreateCounting(5, 0);
  ledQueue = xQueueCreate(10, sizeof(int));
  semaInterrupt = xSemaphoreCreateBinary();
  queueInterrupt = xSemaphoreCreateBinary();
  //TASK INTERRUPT


  // Khởi tạo WiFi
  SetUpWIFI();

  // Khởi tạo NTP client
  timeClient.begin();

  // Khởi tạo TM1637
  tm1637Display.setBrightness(0x0f);

  // Khởi tạo OLED
  SetUpOLED();

  // Khởi tạo semaphore
  xSerialSemaphore = xSemaphoreCreateMutex();
  if (xSerialSemaphore != NULL) {
    xSemaphoreGive(xSerialSemaphore);
  }

  pinMode(BTN_TM1637, INPUT_PULLUP); 
  pinMode(BTN_OLED, INPUT_PULLUP); 

  // Khởi tạo chân LED và PWM
  // ledcAttachPin(LED_PIN, 0); 
  // ledcSetup(0, 5000, 8);

  // Khởi tạo tasks
  xTaskCreatePinnedToCore(TM1637DisplayTask, "TM1637 Task", 2048, NULL, 3, NULL, 1);   
  xTaskCreatePinnedToCore(OLEDDisplayTask, "OLED Task", 2048, NULL, 3, NULL, 1);              
  xTaskCreatePinnedToCore(UDPListenerTask, "UDP Listener Task", 2048, NULL, 3, NULL, 1); 
  // xTaskCreatePinnedToCore(LEDBrightnessTask, "LED Brightness Task", 2048, NULL, 3, NULL, 2);

  //____________________TASK QUEUE - TASK NOFITICATION____________________________
  xTaskCreate(BUTTONCIRTask, "BUTTONCIRTask", 2048, NULL, 1, NULL);
  xTaskCreate(LEDCIRTask, "LEDCIRTask", 2048, NULL, 1, NULL);
  xTaskCreate(ButtonSTTask, "ButtonSTTask", 2048, NULL, 1, NULL);
  xTaskCreate(LEDSTTask, "LEDSTTask", 2048, NULL, 1, &xHandleLEDSTTask);
  //TASK QUEUE - TASK NOFITICATION

  //____________________TASK INTERRUPT____________________________
  xTaskCreatePinnedToCore(ReadSemaButtonTask, "ReadSemaButtonTask", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(WriteSemaLedTask, "WriteSemaLedTask", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(HandleSemaInterruptTask, "HandleSemaInterruptTask", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(ReadQueueButtonTask, "ReadQueueButtonTask", 1024, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(HandleQueueTask, "HandleQueueTask", 1024, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(HandleQueueInterruptTask, "HandleQueueInterruptTask", 1024, NULL, 1, NULL, 1);
  //TASK INTERRUPT

  //____________________TASK Mailbox - TASK Stream Buffers____________________________
  xTaskCreatePinnedToCore(softtimer, "softtimer", 1024, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(readSemaButton, "readSemaButton", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(writeSemaLed, "writeSemaLed", 1024, NULL, 1, NULL, 1);
  xTaskCreate(task1Receiver, "Task1Receiver", 2048, NULL, 1, &task1ReceiverHandle);
  xTaskCreate(task1Sender, "Task1Sender", 2048, NULL, 1, NULL);
  xTaskCreate(task2, "Task2", 2048, NULL, 1, NULL);
  xTaskCreate(task3, "Task3", 2048, NULL, 1, NULL);       
  xTaskCreate(task4, "Task4", 2048, NULL, 1, NULL); 
  xTaskCreate(taskCheckButton2, "TaskCheckButton2", 2048, NULL, 1, NULL);
  xTaskCreate(taskCheckButton3, "TaskCheckButton3", 2048, NULL, 1, NULL);
  xTaskCreate(taskCheckButton4, "TaskCheckButton4", 2048, NULL, 1, NULL);
  //TASK Mailbox - TASK Stream Buffers



}

void loop() {

}

//Hàm kết nối WIFI
void SetUpWIFI(){ 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

//Hàm khởi tạo màn hình OLED
void SetUpOLED(){
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!oledDisplay.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  oledDisplay.clearDisplay();
  oledDisplay.setTextSize(1);
  oledDisplay.setTextColor(SSD1306_WHITE);
}

//Hàm cập nhật thời gian thực từ NTP client
void updateDisplayTime(int* hours, int* minutes, int* seconds) {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  *hours = (epochTime % 86400L) / 3600;
  *minutes = (epochTime % 3600) / 60;
  *seconds = epochTime % 60;
}

// Task hiển thị trên màn hình TM1637
void TM1637DisplayTask(void *pvParameters) {
  unsigned long semaphoreStartTime = 0; 
  bool semaphoreHeld = false; 

  for (;;) {
    static int prevBtnValue = 0; 
    static int prevBtn7Value = 0; 
    int btnValue = digitalRead(BTN_TM1637); 
    if (btnValue == LOW && prevBtnValue == HIGH && !semaphoreHeld) { 
      Serial.println("Button TM1637DisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task TM1637DisplayTask");
        semaphoreHeld = true;
        semaphoreStartTime = millis();
        getTime = 0;
      }
    } else if (btn7Value == 2 && btn7Value != prevBtn7Value) {
      Serial.println("UDP TM1637DisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task UDP TM1637DisplayTask");
        getTime = 0;
        xSemaphoreGive(xSerialSemaphore);
      }
      prevBtn7Value = btn7Value;
    }
    
    if (getTime == 0){
      updateDisplayTime(&hoursOLED, &minutesTM1637, &secondsTM1637);
    }
    
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
    data[0] = tm1637Display.encodeDigit(minutesTM1637 / 10);
    data[1] = tm1637Display.encodeDigit(minutesTM1637 % 10);
    data[2] = tm1637Display.encodeDigit(secondsTM1637 / 10);
    data[3] = tm1637Display.encodeDigit(secondsTM1637 % 10);
    tm1637Display.setSegments(data);
    tm1637Display.showNumberDecEx((minutesTM1637 * 100) + secondsTM1637, 0b01000000, true);
    
    if (semaphoreHeld && (millis() - semaphoreStartTime >= 10000)) {
      xSemaphoreGive(xSerialSemaphore);
      semaphoreHeld = false;
    }
    
    prevBtnValue = btnValue;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Task hiển thị trên màn hình OLED
void OLEDDisplayTask(void *pvParameters) {
  unsigned long semaphoreStartTime = 0; 
  bool semaphoreHeld = false; 

  for (;;) {
    static int prevBtnValue = 0; 
    static int prevBtn8Value = 0; 
    int btnValue = digitalRead(BTN_OLED); 
    if (btnValue == LOW && prevBtnValue == HIGH && !semaphoreHeld) { 
      Serial.println("Button OLEDDisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task OLEDDisplayTask");
        semaphoreHeld = true;
        semaphoreStartTime = millis();
        getTime = 1;
      }
    } else if (btn8Value == 2 && btn8Value != prevBtn8Value) {
      Serial.println("UDP OLEDDisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task UDP OLEDDisplayTask");
        getTime = 1;
        xSemaphoreGive(xSerialSemaphore);
      }
      prevBtn8Value = btn8Value;
    }
    
    if (getTime == 1){
      updateDisplayTime(&hoursOLED, &minutesOLED, &secondsOLED);
    }
    
    oledDisplay.clearDisplay();
    oledDisplay.setTextSize(2);
    oledDisplay.setCursor(0, 10);
    oledDisplay.print(hoursOLED);
    oledDisplay.print(":");
    if (minutesOLED < 10) oledDisplay.print("0");
    oledDisplay.print(minutesOLED);
    oledDisplay.print(":");
    if (secondsOLED < 10) oledDisplay.print("0");
    oledDisplay.print(secondsOLED);
    oledDisplay.display();
    
    if (semaphoreHeld && (millis() - semaphoreStartTime >= 10000)) {
      xSemaphoreGive(xSerialSemaphore);
      semaphoreHeld = false;
    }
    
    prevBtnValue = btnValue;
    vTaskDelay(pdMS_TO_TICKS(500)); 
  }
}

// Xử lý gói tin UDP
void handleUdpPacket(AsyncUDPPacket packet) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, packet.data());

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("arcValue") || doc.containsKey("valueButton7") || doc.containsKey("valueButton8")) {
    arcValue = doc["arcValue"];
    btn7Value = doc["valueButton7"];
    btn8Value = doc["valueButton8"];
    Serial.print("Received arcValue: ");
    Serial.println(arcValue);
    Serial.print("Received valueButton7: ");
    Serial.println(btn7Value);
    Serial.print("Received valueButton8: ");
    Serial.println(btn8Value);

  } else {
    Serial.println("No arcValue found in the packet");
  }

  packet.printf("Got %u bytes of data", packet.length());
}

// Task lắng nghe UDP
void UDPListenerTask(void *pvParameters) {
  if (udp.listen(1234)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      handleUdpPacket(packet);
    });
  }

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Task điều chỉnh độ sáng LED
void LEDBrightnessTask(void *pvParameters) {
  for (;;) {
    int brightness = map(arcValue, 0, 100, 0, 255);  
    ledcWrite(0, brightness);  
    vTaskDelay(pdMS_TO_TICKS(100));  
  }
}

//____________________TASK QUEUE - TASK NOFITICATION____________________________

// Nhiệm vụ nút nhấn cho LED tròn
void BUTTONCIRTask(void *pvParameters) {
    static LedCommand_t currentCase = CASE1;
    pinMode(BUTTON_CIR_PIN, INPUT_PULLUP);

    while (true) {
        if (digitalRead(BUTTON_CIR_PIN) == LOW) {
            vTaskDelay(50 / portTICK_PERIOD_MS); 
            if (digitalRead(BUTTON_CIR_PIN) == LOW) {
                if (xQueueSend(MessageQueue, &currentCase, portMAX_DELAY) == pdPASS) {
                    UBaseType_t messagesWaiting = uxQueueMessagesWaiting(MessageQueue);
                    Serial.print("Messages in queue: ");
                    Serial.println(messagesWaiting);
                    switch (currentCase) {
                        case CASE1:
                            currentCase = CASE2;
                            break;
                        case CASE2:
                            currentCase = CASE3;
                            break;
                        case CASE3:
                            currentCase = CASE1;
                            break;
                    }
                }
                while (digitalRead(BUTTON_CIR_PIN) == LOW) {
                    vTaskDelay(10 / portTICK_PERIOD_MS); 
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Nhiệm vụ nút nhấn cho LED thẳng
void ButtonSTTask(void *pvParameters) {
    pinMode(BUTTON_ST, INPUT_PULLUP);

    while (true) {
        if (digitalRead(BUTTON_ST) == LOW) {
            vTaskDelay(50 / portTICK_PERIOD_MS); 
            if (digitalRead(BUTTON_ST) == LOW) {
                mode = (mode % 3) + 1;
                xTaskNotifyGive(xHandleLEDSTTask); // Gửi thông báo tới LEDSTTask
                while (digitalRead(BUTTON_ST) == LOW) {
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Các hàm điều khiển led tròn cho các chế độ
void handleMode1() {
    uint32_t colors[] = {strip2.Color(255, 0, 0), strip2.Color(0, 255, 0), strip2.Color(0, 0, 255)};
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 3; i++) {
            for (int k = 0; k < strip2.numPixels(); k++) {
                strip2.setPixelColor(k, colors[i]);
            }
            strip2.show();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            strip2.clear();
            strip2.show();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void handleMode2() {
    uint32_t colors[] = {strip2.Color(255, 255, 0), strip2.Color(255, 255, 255), strip2.Color(255, 105, 180)};
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 3; i++) {
            for (int k = 0; k < strip2.numPixels(); k++) {
                strip2.setPixelColor(k, colors[i]);
                strip2.show();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            for (int k = strip2.numPixels() - 1; k >= 0; k--) {
                strip2.setPixelColor(k, 0);
                strip2.show();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    }
}

void handleMode3() {
    uint32_t colors[] = {strip2.Color(255, 255, 0), strip2.Color(0, 255, 0), strip2.Color(255, 255, 255)};
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            for (int k = 0; k < strip2.numPixels(); k++) {
                strip2.clear();
                strip2.setPixelColor(k, colors[i]);
                strip2.show();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    }
    strip2.clear(); 
    strip2.show();
}

// Các hàm điều khiển LED thẳng cho các chế độ
void case1Pattern() {
    for (int i = 0; i < 2; i++) {
        strip1.fill(strip1.Color(255, 0, 0)); // Red
        strip1.show();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        strip1.clear();
        strip1.show();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        strip1.fill(strip1.Color(0, 0, 255)); // Blue
        strip1.show();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        strip1.clear();
        strip1.show();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        strip1.fill(strip1.Color(0, 255, 0)); // Green
        strip1.show();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        strip1.clear();
        strip1.show();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void case2Pattern() {
    for (int j = 0; j < 2; j++) {
        uint32_t colors[] = {strip1.Color(255, 255, 255), strip1.Color(255, 255, 0), strip1.Color(0, 255, 0)}; // White, Yellow, Green
        for (int k = 0; k < 3; k++) {
            for (int i = 0; i < NUMCIR; i++) {
                strip1.setPixelColor(i, colors[k]);
                strip1.show();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            for (int i = NUMCIR - 1; i >= 0; i--) {
                strip1.setPixelColor(i, 0);
                strip1.show();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    }
}

void case3Pattern() {
    uint32_t colors[] = {strip1.Color(255, 0, 0), strip1.Color(255, 255, 0), strip1.Color(255, 255, 255)}; // Red, Yellow, White

    for (int j = 0; j < 2; j++) {
        for (int c = 0; c < 3; c++) {
            for (int i = 0; i < NUMCIR; i++) {
                strip1.clear();
                strip1.setPixelColor(i, colors[c]);
                strip1.show();
                vTaskDelay(150 / portTICK_PERIOD_MS);
            }
            for (int i = NUMCIR - 1; i >= 0; i--) {
                strip1.setPixelColor(i, 0);
                strip1.show();
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }
}

// Nhiệm vụ LED tròn
void LEDCIRTask(void *pvParameters) {
    LedCommand_t receivedCommand;
    while (true) {
        if (xQueueReceive(MessageQueue, &receivedCommand, portMAX_DELAY) == pdPASS) {
            UBaseType_t messagesWaiting = uxQueueMessagesWaiting(MessageQueue);
            Serial.print("Messages remaining in queue: ");
            Serial.println(messagesWaiting);
            switch (receivedCommand) {
                case CASE1:
                    case1Pattern();
                    break;
                case CASE2:
                    case2Pattern();
                    break;
                case CASE3:
                    case3Pattern();
                    break;
                default:
                    break;
            }
        }
    }
}

// Nhiệm vụ LED thẳng
void LEDSTTask(void *pvParameters) {
    uint32_t ulNotificationValue;

    while (1) {
        ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Chờ thông báo
        if (ulNotificationValue == 1) {
            switch (mode) {
                case 1:
                    handleMode1();
                    break;
                case 2:
                    handleMode2();
                    break;
                case 3:
                    handleMode3();
                    break;
                default:
                    strip2.clear();
                    strip2.show();
                    break;
            }
        } else {
            strip2.clear();
            strip2.show();
        }
    }
}

//TASK QUEUE - TASK NOFITICATION

//____________________TASK INTERRUPT____________________________
void IRAM_ATTR HandleSemaInterrupt() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(semaInterrupt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IRAM_ATTR HandleQueueInterrupt() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(queueInterrupt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ReadSemaButtonTask(void *pvParameters) {
    while (1) {
        int buttonValue = digitalRead(BUTTON_SEMA);
        static int prevButtonValue = HIGH;
        if (buttonValue == LOW && prevButtonValue == HIGH) {
            xSemaphoreGive(countingSemaLed5);
        }
        prevButtonValue = buttonValue;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void WriteSemaLedTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(countingSemaLed5, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < 3; i++) {
                digitalWrite(LED_SEMA_PIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(500));
                digitalWrite(LED_SEMA_PIN, LOW);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
    }
}

void HandleSemaInterruptTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(semaInterrupt, portMAX_DELAY) == pdTRUE) {
            ledInterruptSemaState = !ledInterruptSemaState; // Đảo trạng thái LED ngắt
            digitalWrite(LED_INTERRUPT_SEMA_PIN, ledInterruptSemaState); // Cập nhật trạng thái LED ngắt
        }
    }
}

void ReadQueueButtonTask(void *pvParameters) {
    while (1) {
        int buttonValue = digitalRead(BUTTON_QUEUE);
        static int prevButtonValue = HIGH;
        if (buttonValue == LOW && prevButtonValue == HIGH) {
            int msg = 1;
            xQueueSend(ledQueue, &msg, portMAX_DELAY);
        }
        prevButtonValue = buttonValue;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void HandleQueueTask(void *pvParameters) {
    int msg;
    while (1) {
        if (xQueueReceive(ledQueue, &msg, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < 10; i++) {
                digitalWrite(LED_QUEUE_PIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(500));
                digitalWrite(LED_QUEUE_PIN, LOW);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
    }
}

void HandleQueueInterruptTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(queueInterrupt, portMAX_DELAY) == pdTRUE) {
            ledInterruptQueueState = !ledInterruptQueueState; // Đảo trạng thái LED ngắt
            digitalWrite(LED_INTERRUPT_QUEUE_PIN, ledInterruptQueueState); // Cập nhật trạng thái LED ngắt
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
//TASK INTERRUPT

//____________________TASK Mailbox - TASK Stream Buffers____________________________
// Hàm thiết lập màu cho dải LED WS2812 dựa trên ba giá trị màu RGB
void setWS2812Color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < 4; i++) {
        ws2812.setPixelColor(i, ws2812.Color(r, g, b));
    }
    ws2812.show();
}

void vTimerCallback(TimerHandle_t xTimer) {
  P.displayText("K", PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!P.displayAnimate()) {
    // Do nothing until animation is completed
  }
  P.displayClear();
}

void softtimer(void *pvParameters) {
  TimerHandle_t xTimer = xTimerCreate("LEDTimer", pdMS_TO_TICKS(2000), pdTRUE, NULL, vTimerCallback);

  if (xTimer == NULL) {
    // Timer was not created
    for (;;);
  }

  if (xTimerStart(xTimer, 0) != pdPASS) {
    // Timer could not be started
    for (;;);
  }

  // Suspend this task as it's only needed for timer initialization
  vTaskSuspend(NULL);
}

void readSemaButton(void *pvParameters) {
    static int prevButtonValue = HIGH; // Initial previous button value is HIGH (not pressed)
    while (1) {
        int buttonValue = digitalRead(BUTTON_SEMA); // Read the current state of BUTTON_SEMA
        if (buttonValue == LOW && prevButtonValue == HIGH) { // If the button is pressed (active low)
            xSemaphoreGive(countingSemaLed5); // Increment the semaphore count
        }
        prevButtonValue = buttonValue; // Save the current button state for the next loop
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay for 10ms to debounce the button
    }
}

void writeSemaLed(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(countingSemaLed5, portMAX_DELAY) == pdTRUE) { // Wait indefinitely for the semaphore
            for (int i = 0; i < 3; i++) { // Blink the LED 3 times
                digitalWrite(LED2_PIN, HIGH); // Turn on the LED
                vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500ms
                digitalWrite(LED2_PIN, LOW); // Turn off the LED
                vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500ms
            }
        }
    }
}

void task1Sender(void *pvParameters) {
    uint8_t currentState = 0;
    static uint8_t lastButtonState = HIGH;
    static uint8_t button1PressCount = 0; // Biến đếm số lần nút BUTTON1 được nhấn

    while (1) {
        uint8_t currentButtonState = digitalRead(BUTTON1_PIN);
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            button1PressCount++;
            if (button1PressCount > 4) {
                button1PressCount = 1;
            }
            Serial.print("1--- Button 1 has been pressed ");
            Serial.print(button1PressCount);
            Serial.println(" times.");

            currentState = (currentState + 1) % 4;
            // Gửi trạng thái LED mới tới task1Receiver
            xTaskNotify(task1ReceiverHandle, currentState, eSetValueWithOverwrite);
            vTaskDelay(pdMS_TO_TICKS(200)); // Đợi 200ms trước khi kiểm tra lại
        }
        lastButtonState = currentButtonState;
        vTaskDelay(pdMS_TO_TICKS(10)); // Đợi 10ms trước khi kiểm tra trạng thái nút tiếp theo
    }
}

void task1Receiver(void *pvParameters) {
    uint32_t receivedState;
    while (1) {
        // Chờ nhận thông báo từ task1Sender
        xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedState, portMAX_DELAY);
        switch (receivedState) {
            case 0:
                digitalWrite(LED_R_PIN, LOW);
                digitalWrite(LED_G_PIN, LOW);
                digitalWrite(LED_B_PIN, LOW);
                break;
            case 1:
                digitalWrite(LED_R_PIN, HIGH);
                digitalWrite(LED_G_PIN, LOW);
                digitalWrite(LED_B_PIN, LOW);
                break;
            case 2:
                digitalWrite(LED_R_PIN, LOW);
                digitalWrite(LED_G_PIN, HIGH);
                digitalWrite(LED_B_PIN, LOW);
                break;
            case 3:
                digitalWrite(LED_R_PIN, LOW);
                digitalWrite(LED_G_PIN, LOW);
                digitalWrite(LED_B_PIN, HIGH);
                break;
        }
    }
}

void task2(void* pvParameters) {
    uint8_t event;
    while (1) {
        // Chờ nhận sự kiện từ hàng đợi task2Queue
        if (xQueueReceive(task2Queue, &event, portMAX_DELAY) == pdTRUE) {
            Serial.println("Event received in task2");
            // Cố gắng lấy semaphore để đồng bộ hóa truy cập vào WS2812
            if (xSemaphoreTake(ws2812Mutex, portMAX_DELAY) == pdTRUE) {
                Serial.println("Semaphore taken in task2");
                // Xử lý sự kiện dựa trên trạng thái hiện tại của WS2812
                switch (ws2812State) {
                    case WS2812_OFF:
                        setWS2812Color(0, 255, 0); // Xanh lá
                        ws2812State = WS2812_GREEN;
                        Serial.println("WS2812 set to GREEN");
                        break;
                    case WS2812_GREEN:
                        setWS2812Color(0, 0, 255); // Xanh dương
                        ws2812State = WS2812_BLUE;
                        Serial.println("WS2812 set to BLUE");
                        break;
                    case WS2812_BLUE:
                        setWS2812Color(255, 255, 255); // Trắng
                        ws2812State = WS2812_WHITE;
                        Serial.println("WS2812 set to WHITE");
                        break;
                    default:
                        setWS2812Color(0, 0, 0); // Tắt
                        ws2812State = WS2812_OFF;
                        Serial.println("WS2812 set to OFF");
                        break;
                }
                // Trả lại semaphore sau khi cập nhật WS2812
                xSemaphoreGive(ws2812Mutex);
                Serial.println("Semaphore released in task2");
            } else {
                Serial.println("Failed to take semaphore in task2");
            }
        } else {
            Serial.println("Failed to receive event in task2");
        }
    }
}

void taskCheckButton2(void *pvParameters) {
    static uint8_t lastButton2State = HIGH;
    uint8_t currentButton2State;
    uint8_t event = 1;

    while (1) {
        currentButton2State = digitalRead(BUTTON2_PIN);
        if (currentButton2State == LOW && lastButton2State == HIGH) {
            Serial.print("2--- Messages waiting in task2Queue before sending: ");
            Serial.println(uxQueueMessagesWaiting(task2Queue));

            Serial.println("2--- Button 2 pressed, sending event to task2Queue");
            if (xQueueSend(task2Queue, &event, pdMS_TO_TICKS(100)) == pdPASS) {
                Serial.println("2--- Event sent to task2Queue successfully");
            } else {
                Serial.println("2--- Failed to send event to task2Queue");
            }
        }
        lastButton2State = currentButton2State;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task3(void* pvParameters) {
    uint8_t event;
    while (1) {
        if (xQueueReceive(task3Queue, &event, portMAX_DELAY) == pdTRUE) {
            Serial.println("Event received in task3");
            if (xSemaphoreTake(ws2812Mutex, portMAX_DELAY) == pdTRUE) {
                do {
                    setWS2812Color(255, 0, 0); // Đặt màu đỏ
                    vTaskDelay(pdMS_TO_TICKS(5000)); // delay 5s
                    setWS2812Color(0, 0, 0); // Tắt LED
                    // Kiểm tra xem còn sự kiện nào trong hàng đợi không
                    if (uxQueueMessagesWaiting(task3Queue) == 0) {
                        xSemaphoreGive(ws2812Mutex); // Chỉ giải phóng khi không còn sự kiện
                        Serial.println("Semaphore released in task3, no more events.");
                        break;
                    } else {
                        Serial.println("Processing next event in task3 queue.");
                    }
                } while (xQueueReceive(task3Queue, &event, 0) == pdTRUE); // Xử lý ngay sự kiện tiếp theo nếu có
            }
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void taskCheckButton3(void *pvParameters) {
    static uint8_t lastButton3State = HIGH;
    uint8_t currentButton3State;
    uint8_t event = 1;

    while (1) {
        currentButton3State = digitalRead(BUTTON3_PIN);
        if (currentButton3State == LOW && lastButton3State == HIGH) {
            Serial.print("3--- Messages waiting in task3Queue before sending: ");
            Serial.println(uxQueueMessagesWaiting(task3Queue));

            Serial.println("3--- Button 3 pressed, sending event to task3Queue");
            if (xQueueSend(task3Queue, &event, pdMS_TO_TICKS(100)) == pdPASS) {
                Serial.println("3--- Event sent to task3Queue successfully");
            } else {
                Serial.println("3--- Failed to send event to task3Queue");
            }
        }
        lastButton3State = currentButton3State;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void taskCheckButton4(void *pvParameters) {
    uint8_t buttonState = 0;
    uint8_t lastButtonState = HIGH; 
    char colorData[60]; // Mảng chứa dữ liệu gửi

    while (1) {
        buttonState = digitalRead(BUTTON4_PIN);
        if (buttonState == LOW && lastButtonState == HIGH) {
            // Tạo chuỗi màu theo yêu cầu
            strcpy(colorData, "0,0,255|0,255,0|255,0,0|0,255,255|255,0,255|255,255,0");
            xStreamBufferSend(xStreamBuffer, (void *)colorData, strlen(colorData), portMAX_DELAY);
        }
        lastButtonState = buttonState; 
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task4(void *pvParameters) {
    size_t xReceivedBytes;
    char rcvColorData[60]; 
    int blinkDelay = 500; // Thời gian delay giữa các lần nhấp nháy (500ms)
    char* token;
    int r, g, b;

    while (1) {
        xReceivedBytes = xStreamBufferReceive(xStreamBuffer, (void *)rcvColorData, sizeof(rcvColorData), portMAX_DELAY);
        if (xReceivedBytes > 0) {
            token = strtok(rcvColorData, "|");
            while (token != NULL) {
                sscanf(token, "%d,%d,%d", &r, &g, &b);
                // Bật LED với màu nhận được
                digitalWrite(LED_R, r > 0 ? HIGH : LOW);
                digitalWrite(LED_G, g > 0 ? HIGH : LOW);
                digitalWrite(LED_B, b > 0 ? HIGH : LOW);
                Serial.print("R: ");
                Serial.print(r);
                Serial.print(", G: ");
                Serial.print(g);
                Serial.print(", B: ");
                Serial.println(b);
                vTaskDelay(pdMS_TO_TICKS(blinkDelay)); 

                // Tắt LED
                digitalWrite(LED_R, LOW);
                digitalWrite(LED_G, LOW);
                digitalWrite(LED_B, LOW);
                vTaskDelay(pdMS_TO_TICKS(blinkDelay)); // Đợi nhận màu tiếp theo

                token = strtok(NULL, "|"); // Lấy màu tiếp theo trong chuỗi
            }
        }
    }
}
//TASK Mailbox - TASK Stream Buffers