#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/stream_buffer.h"
#include "Adafruit_NeoPixel.h"
#include "stdbool.h"

// Led RGB
#define LED_R_PIN 4 
#define LED_G_PIN 16 
#define LED_B_PIN 17 
// Led ws2812
#define WS2812_PIN 5 
// Led SMD RGB KY-009
#define LED_R 12
#define LED_G 14
#define LED_B 27
// Định nghĩa chân GPIO cho Button
#define BUTTON1_PIN 15
#define BUTTON2_PIN 2
#define BUTTON3_PIN 0
#define BUTTON4_PIN 18

// Định nghĩa các hằng số cho trạng thái bật và tắt của LED
#define LED_ON true
#define LED_OFF false

// Khai báo ws2812Mutex ở phạm vi toàn cục
SemaphoreHandle_t ws2812Mutex;

// Xử lý sự kiện 1------------------------------------------------------------------------------------------------------------

// Khai báo task handle cho task1Receiver để có thể gửi thông báo tới nó
TaskHandle_t task1ReceiverHandle = NULL;

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


// Xử lý sự kiện 2------------------------------------------------------------------------------------------------------------

// Hàng đợi dùng để gửi các sự kiện từ nút bấm BUTTON2 đến task2
QueueHandle_t task2Queue;

// Khởi tạo đối tượng ledRGB và ws2812 với số lượng pixel và chân GPIO tương ứng
Adafruit_NeoPixel ws2812(4, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// Định nghĩa các trạng thái của LED RGB thông thường và LED WS2812
enum WS2812State { WS2812_RED, WS2812_GREEN, WS2812_BLUE, WS2812_WHITE, WS2812_OFF };
WS2812State ws2812State = WS2812_OFF;

// Hàm thiết lập màu cho dải LED WS2812 dựa trên ba giá trị màu RGB
void setWS2812Color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < 4; i++) {
        ws2812.setPixelColor(i, ws2812.Color(r, g, b));
    }
    ws2812.show();
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

// Task kiểm tra BUTTON2
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

// Xử lý sự kiện 3------------------------------------------------------------------------------------------------------------

// Hàng đợi dùng để gửi các sự kiện từ nút bấm BUTTON3 đến task3
QueueHandle_t task3Queue;

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

// Task kiểm tra BUTTON3
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

// Xử lý sự kiện 4------------------------------------------------------------------------------------------------------------

StreamBufferHandle_t xStreamBuffer;

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

/*---------------------------------------------------------------------------------------------------------------------------*/
void setup() {
    Serial.begin(9600); 

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

    ws2812Mutex = xSemaphoreCreateMutex();
    task2Queue = xQueueCreate(2, sizeof(uint8_t)); 
    task3Queue = xQueueCreate(2, sizeof(uint8_t));
    xStreamBuffer = xStreamBufferCreate(1028, 1); 

    xTaskCreate(task1Receiver, "Task1Receiver", 2048, NULL, 1, &task1ReceiverHandle);
    xTaskCreate(task1Sender, "Task1Sender", 2048, NULL, 1, NULL);
    xTaskCreate(task2, "Task2", 2048, NULL, 1, NULL);
    xTaskCreate(task3, "Task3", 2048, NULL, 1, NULL);       
    xTaskCreate(task4, "Task4", 2048, NULL, 1, NULL); 
    xTaskCreate(taskCheckButton2, "TaskCheckButton2", 2048, NULL, 1, NULL);
    xTaskCreate(taskCheckButton3, "TaskCheckButton3", 2048, NULL, 1, NULL);
    xTaskCreate(taskCheckButton4, "TaskCheckButton4", 2048, NULL, 1, NULL);

}

void loop() {
}
