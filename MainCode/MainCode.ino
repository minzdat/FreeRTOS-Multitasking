#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Adafruit_NeoPixel.h"
#include "stdbool.h"

// Định nghĩa chân GPIO cho LED
#define LED_R_PIN 4 // Màu đỏ ledRGB 
#define LED_G_PIN 16 // Màu xanh lá ledRGB 
#define LED_B_PIN 17 // Màu xanh dương ledRGB 
#define WS2812_PIN 5 // Led ws2812

// Định nghĩa chân GPIO cho Button
#define BUTTON1_PIN 15
#define BUTTON2_PIN 2
#define BUTTON3_PIN 0

// Khởi tạo đối tượng ledRGB và ws2812 với số lượng pixel và chân GPIO tương ứng
Adafruit_NeoPixel ledRGB(1, LED_R_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ws2812(4, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// Định nghĩa các hằng số cho trạng thái bật và tắt của LED
#define LED_ON true
#define LED_OFF false

// Định nghĩa các trạng thái của LED RGB thông thường và LED WS2812
enum LEDState { RED, GREEN, BLUE, OFF };
LEDState ledState = OFF;
enum WS2812State { WS2812_RED, WS2812_GREEN, WS2812_BLUE, WS2812_WHITE, WS2812_OFF };
WS2812State ws2812State = WS2812_OFF;

// Hàm thiết lập màu cho LED RGB thông thường dựa trên ba giá trị boolean cho màu đỏ, xanh lá, và xanh dương
void setLEDColor(bool r, bool g, bool b) {
    digitalWrite(LED_R_PIN, r ? HIGH : LOW);
    digitalWrite(LED_G_PIN, g ? HIGH : LOW);
    digitalWrite(LED_B_PIN, b ? HIGH : LOW);
}

// Hàm thiết lập màu cho dải LED WS2812 dựa trên ba giá trị màu RGB
void setWS2812Color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < 4; i++) {
        ws2812.setPixelColor(i, ws2812.Color(r, g, b));
    }
    ws2812.show();
}

// Khai báo ws2812Mutex ở phạm vi toàn cục
SemaphoreHandle_t ws2812Mutex;

// Hàng đợi dùng để gửi các sự kiện từ nút bấm BUTTON1 đến task1Receiver
QueueHandle_t task1Mailbox;
// Hàng đợi dùng để gửi các sự kiện từ nút bấm BUTTON2 đến task2
QueueHandle_t task2Queue;
// Hàng đợi dùng để gửi các sự kiện từ nút bấm BUTTON3 đến task3
QueueHandle_t task3Queue;


// Xử lý sự kiện BUTTON1

static uint8_t button1PressCount = 0; // Biến đếm số lần nút BUTTON1 được nhấn

void task1Sender(void *pvParameters) {
    LEDState currentState = OFF; // Trạng thái LED hiện tại, bắt đầu ở trạng thái tắt
    static uint8_t lastButtonState = HIGH; // Lưu trạng thái cuối cùng của nút, bắt đầu ở trạng thái không nhấn

    while (1) {
        uint8_t currentButtonState = digitalRead(BUTTON1_PIN); // Đọc trạng thái hiện tại của nút BUTTON1
        if (currentButtonState == LOW && lastButtonState == HIGH) { // Kiểm tra sự kiện nhấn nút
            currentState = (LEDState)((currentState + 1) % 4); // Thay đổi trạng thái LED
            uint8_t event = currentState; // Gửi trạng thái LED mới tới hàng đợi
            xQueueSend(task1Mailbox, &event, 0); // Gửi trạng thái LED tới task1Receiver
            vTaskDelay(pdMS_TO_TICKS(200)); // Đợi 200ms trước khi kiểm tra lại
        }
        lastButtonState = currentButtonState; // Cập nhật trạng thái cuối cùng của nút
        vTaskDelay(pdMS_TO_TICKS(10)); // Đợi 10ms trước khi kiểm tra trạng thái nút tiếp theo
    }
}

void task1Receiver(void *pvParameters) {
    uint8_t receivedMessage;
    while (1) {
        if (xQueueReceive(task1Mailbox, &receivedMessage, portMAX_DELAY) == pdTRUE) {
            // Xử lý thông điệp để thiết lập màu LED
            switch (receivedMessage) {
                case RED:
                    setLEDColor(true, false, false);
                    break;
                case GREEN:
                    setLEDColor(false, true, false);
                    break;
                case BLUE:
                    setLEDColor(false, false, true);
                    break;
                case OFF:
                    setLEDColor(false, false, false);
                    break;
            }
        }
    }
}

// Xử lý sự kiện BUTTON2

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

// Xử lý sự kiện BUTTON3

void task3(void* pvParameters) {
    uint8_t event;
    while (1) {
        if (xQueueReceive(task3Queue, &event, portMAX_DELAY) == pdTRUE) {
            Serial.println("Event received in task3");
            if (xSemaphoreTake(ws2812Mutex, portMAX_DELAY) == pdTRUE) {
                setWS2812Color(255, 0, 0); // Đặt màu đỏ
                vTaskDelay(5000 / portTICK_PERIOD_MS); 
                setWS2812Color(0, 0, 0); // Tắt LED
                xSemaphoreGive(ws2812Mutex);
                Serial.println("WS2812 set to RED then turned off in task3");
            }
            vTaskDelay(200 / portTICK_PERIOD_MS); 
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Đợi trước khi kiểm tra lại hàng đợi
    }
}

// Hàm setup khởi tạo các chân GPIO, hàng đợi, semaphore và tạo các task
void setup() {
    pinMode(LED_R_PIN, OUTPUT); // Thiết lập chân cho LED đỏ
    pinMode(LED_G_PIN, OUTPUT); // Thiết lập chân cho LED xanh lá
    pinMode(LED_B_PIN, OUTPUT); // Thiết lập chân cho LED xanh dương
    pinMode(WS2812_PIN, OUTPUT); // Thiết lập chân cho dải LED WS2812
    pinMode(BUTTON1_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 1
    pinMode(BUTTON2_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 2
    pinMode(BUTTON3_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 3

    task1Mailbox = xQueueCreate(1, sizeof(uint8_t)); // Tạo hàng đợi cho task1
    task2Queue = xQueueCreate(2, sizeof(uint8_t)); // Tạo hàng đợi cho task2
    task3Queue = xQueueCreate(2, sizeof(uint8_t)); // Tạo hàng đợi cho task3

    ws2812Mutex = xSemaphoreCreateMutex(); // Tạo semaphore cho WS2812
    
    xTaskCreate(task1Sender, "Task1Sender", 2048, NULL, 1, NULL); // Tạo task1Sender
    xTaskCreate(task1Receiver, "Task1Receiver", 2048, NULL, 1, NULL); // Tạo task1Receiver
    xTaskCreate(task2, "task2", 2048, NULL, 1, NULL); // Tạo task2
    xTaskCreate(task3, "task3", 2048, NULL, 1, NULL); // Tạo task3
   
    Serial.begin(9600); // Khởi động giao tiếp Serial
}

void loop() {
    static uint8_t lastButton1State = HIGH;
    static uint8_t lastButton2State = HIGH;
    static uint8_t lastButton3State = HIGH;
    uint8_t currentButton1State = digitalRead(BUTTON1_PIN);
    uint8_t currentButton2State = digitalRead(BUTTON2_PIN);
    uint8_t currentButton3State = digitalRead(BUTTON3_PIN);
    uint8_t event = 1;

    // Kiểm tra BUTTON1
    if (currentButton1State == LOW && lastButton1State == HIGH) {
        button1PressCount++;
        if (button1PressCount > 4) {
            button1PressCount = 1;
        }
        Serial.print("1--- Button 1 has been pressed ");
        Serial.print(button1PressCount);
        Serial.println(" times.");
        xQueueSend(task1Mailbox, &event, 0);
        vTaskDelay(pdMS_TO_TICKS(300)); 
    }   
    lastButton1State = currentButton1State;

    // Kiểm tra BUTTON2
    if (currentButton2State == LOW && lastButton2State == HIGH) {
        Serial.print("2--- Messages waiting in task2Queue before sending: ");
        Serial.println(uxQueueMessagesWaiting(task2Queue));

        Serial.println("2--- Button 2 pressed, sending event to task2Queue");
        if (xQueueSend(task2Queue, &event, pdMS_TO_TICKS(100)) == pdPASS) {
            Serial.println("2--- Event sent to task2Queue successfully");
        } else {
            Serial.println("2--- Failed to send event to task2Queue");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    lastButton2State = currentButton2State;

    // Kiểm tra BUTTON3
    if (currentButton3State == LOW && lastButton3State == HIGH) {
        Serial.print("3--- Messages waiting in task3Queue before sending: ");
        Serial.println(uxQueueMessagesWaiting(task3Queue));

        Serial.println("3--- Button 3 pressed, sending event to task3Queue");
        if (xQueueSend(task3Queue, &event, pdMS_TO_TICKS(100)) == pdPASS) {
            Serial.println("3--- Event sent to task3Queue successfully");
        } else {
            Serial.println("3--- Failed to send event to task3Queue");
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    lastButton3State = currentButton3State;
}
