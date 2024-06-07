#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "AsyncUDP.h" 
#include <ArduinoJson.h>


// ID và password WiFi
const char* ssid = "The Binh";
const char* password = "03030303";

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
Adafruit_SSD1306 oledDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

void TM1637DisplayTask(void *pvParameters);
void OLEDDisplayTask(void *pvParameters);
void UDPListenerTask(void *pvParameters); 

void SetUpWIFI();
void SetUpOLED();
void updateDisplayTime(int* hours, int* minutes, int* seconds);
void handleUdpPacket(AsyncUDPPacket packet);

void setup() {
  Serial.begin(115200);

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

  // Khởi tạo tasks
  xTaskCreatePinnedToCore(TM1637DisplayTask, "TM1637 Task", 2048, NULL, 3, NULL, 1);   
  xTaskCreatePinnedToCore(OLEDDisplayTask, "OLED Task", 2048, NULL, 3, NULL, 1);              
  xTaskCreatePinnedToCore(UDPListenerTask, "UDP Listener Task", 2048, NULL, 3, NULL, 1); 

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
    static int prevArcValue = 0; 
    int btnValue = digitalRead(BTN_TM1637); 
    if (btnValue == LOW && prevBtnValue == HIGH && !semaphoreHeld) { 
      Serial.println("Button TM1637DisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task TM1637DisplayTask");
        semaphoreHeld = true;
        semaphoreStartTime = millis();
        getTime = 0;
      }
    } else if (arcValue < 50 && arcValue != prevArcValue) {
      Serial.println("UDP TM1637DisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task UDP TM1637DisplayTask");
        getTime = 0;
        xSemaphoreGive(xSerialSemaphore);
      }
      prevArcValue = arcValue;
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
    static int prevArcValue = 0; 
    int btnValue = digitalRead(BTN_OLED); 
    if (btnValue == LOW && prevBtnValue == HIGH && !semaphoreHeld) { 
      Serial.println("Button OLEDDisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task OLEDDisplayTask");
        semaphoreHeld = true;
        semaphoreStartTime = millis();
        getTime = 1;
      }
    } else if (arcValue > 50 && arcValue != prevArcValue) {
      Serial.println("UDP OLEDDisplayTask");
      if (xSemaphoreTake(xSerialSemaphore, portMAX_DELAY) == pdTRUE) {
        Serial.println("Task UDP OLEDDisplayTask");
        getTime = 1;
        xSemaphoreGive(xSerialSemaphore);
      }
      prevArcValue = arcValue;
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

  if (doc.containsKey("arcValue")) {
    arcValue = doc["arcValue"];
    Serial.print("Received arcValue: ");
    Serial.println(arcValue);

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