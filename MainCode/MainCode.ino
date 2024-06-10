#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Adafruit_NeoPixel.h"
#include "stdbool.h"

// Led RGB
#define LED_R_PIN 4 
#define LED_G_PIN 16 
#define LED_B_PIN 17 
// Led ws2812
#define WS2812_PIN 5 
//Led SMD RGB 
#define LED_R 12
#define LED_G 14
#define LED_B 27
// Định nghĩa chân GPIO cho Button
#define BUTTON1_PIN 15
#define BUTTON2_PIN 2
#define BUTTON3_PIN 0
#define BUTTON4_PIN 21
// Định nghĩa các hằng số cho trạng thái bật và tắt của LED
#define LED_ON true
#define LED_OFF false

// Khai báo ws2812Mutex ở phạm vi toàn cục
SemaphoreHandle_t ws2812Mutex;

// Xử lý sự kiện 1------------------------------------------------------------------------------------------------------------

// Hàng đợi dùng để gửi các sự kiện từ nút bấm BUTTON1 đến task1Receiver
QueueHandle_t task1Mailbox;

static uint8_t button1PressCount = 0; // Biến đếm số lần nút BUTTON1 được nhấn

void task1Sender(void *pvParameters) {
    uint8_t currentState = 0; // Trạng thái LED hiện tại, bắt đầu ở trạng thái tắt
    static uint8_t lastButtonState = HIGH; // Lưu trạng thái cuối cùng của nút, bắt đầu ở trạng thái không nhấn

    while (1) {
        uint8_t currentButtonState = digitalRead(BUTTON1_PIN); // Đọc trạng thái hiện tại của nút BUTTON1
        if (currentButtonState == LOW && lastButtonState == HIGH) { // Kiểm tra sự kiện nhấn nút
            currentState = (currentState + 1) % 4; // Thay đổi trạng thái LED
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
                case 0: // OFF
                    digitalWrite(LED_R_PIN, LOW);
                    digitalWrite(LED_G_PIN, LOW);
                    digitalWrite(LED_B_PIN, LOW);
                    break;
                case 1: // RED
                    digitalWrite(LED_R_PIN, HIGH);
                    digitalWrite(LED_G_PIN, LOW);
                    digitalWrite(LED_B_PIN, LOW);
                    break;
                case 2: // GREEN
                    digitalWrite(LED_R_PIN, LOW);
                    digitalWrite(LED_G_PIN, HIGH);
                    digitalWrite(LED_B_PIN, LOW);
                    break;
                case 3: // BLUE
                    digitalWrite(LED_R_PIN, LOW);
                    digitalWrite(LED_G_PIN, LOW);
                    digitalWrite(LED_B_PIN, HIGH);
                    break;
            }
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

// Xử lý sự kiện 4------------------------------------------------------------------------------------------------------------
SemaphoreHandle_t mutexHandle;

//Giải pháp Paterson
volatile bool taskSelect = false; // false cho Task1, true cho Task2
volatile bool flag0 = false, flag1 = false; // Flag cho Peterson
volatile int turn;

void task4_1(void *pvParameters) {
    while (1) {
        if (!taskSelect) {
            flag0 = true;
            turn = 1;
            while (flag1 && turn == 1) {
                // Busy waiting
            }
            // Critical section
            digitalWrite(LED_R, LOW);
            digitalWrite(LED_B, LOW);
            digitalWrite(LED_G, HIGH);  // Bật LED xanh lá
            vTaskDelay(pdMS_TO_TICKS(500));
            digitalWrite(LED_G, LOW);  // Tắt LED xanh lá
            vTaskDelay(pdMS_TO_TICKS(500));
            // End of critical section
            flag0 = false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void task4_2(void *pvParameters) {
    uint8_t color = 0;
    while (1) {
        if (taskSelect) {
            flag1 = true;
            turn = 0;
            while (flag0 && turn == 0) {
                // Busy waiting
            }
            // Critical section
            switch (color % 7) {  
                case 0:
                    digitalWrite(LED_R, HIGH);  // Đỏ
                    break;
                case 1:
                    digitalWrite(LED_G, HIGH);  // Xanh lá
                    break;
                case 2:
                    digitalWrite(LED_B, HIGH);  // Xanh dương
                    break;
                case 3:
                    digitalWrite(LED_R, HIGH);
                    digitalWrite(LED_G, HIGH);  // Vàng
                    break;
                case 4:
                    digitalWrite(LED_R, HIGH);
                    digitalWrite(LED_B, HIGH);  // Tím
                    break;
                case 5:
                    digitalWrite(LED_G, HIGH);
                    digitalWrite(LED_B, HIGH);  // Xanh ngọc
                    break;
                case 6:
                    digitalWrite(LED_R, HIGH);
                    digitalWrite(LED_G, HIGH);
                    digitalWrite(LED_B, HIGH);  // Trắng
                    break;
            }
            color++;  // Tăng biến color để thay đổi màu tiếp theo
            vTaskDelay(pdMS_TO_TICKS(1000));
            // End of critical section
            flag1 = false;
            digitalWrite(LED_R, LOW);
            digitalWrite(LED_G, LOW);
            digitalWrite(LED_B, LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void switchTask4(void *pvParameters) {
    static uint8_t lastButton4State = digitalRead(BUTTON4_PIN); // Khởi tạo trạng thái ban đầu phù hợp với trạng thái thực tế của nút
    uint8_t currentButton4State;

    while (1) {
        currentButton4State = digitalRead(BUTTON4_PIN);

        if (currentButton4State == LOW && lastButton4State == HIGH) {
            Serial.println("Button 4 pressed, toggling task select");
            if (xSemaphoreTake(mutexHandle, portMAX_DELAY)) {
                taskSelect = !taskSelect;
                xSemaphoreGive(mutexHandle);
                Serial.print("Task select is now: ");
                Serial.println(taskSelect ? "Task 4_2" : "Task 4_1");
            } else {
                Serial.println("Failed to take mutex in task4");
            }
        }
        lastButton4State = currentButton4State; // Cập nhật trạng thái cuối cùng sau khi xử lý
        vTaskDelay(pdMS_TO_TICKS(50)); // Đợi một khoảng thời gian ngắn trước khi kiểm tra lại
    }
}

void setup() {
    pinMode(LED_R_PIN, OUTPUT); // Thiết lập chân cho LED đỏ
    pinMode(LED_G_PIN, OUTPUT); // Thiết lập chân cho LED xanh lá
    pinMode(LED_B_PIN, OUTPUT); // Thiết lập chân cho LED xanh dương
    pinMode(WS2812_PIN, OUTPUT); // Thiết lập chân cho dải LED WS2812
    pinMode(LED_R, OUTPUT); // Thiết lập chân cho LED đỏ
    pinMode(LED_G, OUTPUT); // Thiết lập chân cho LED xanh lá
    pinMode(LED_B, OUTPUT); // Thiết lập chân cho LED xanh dương
    pinMode(BUTTON1_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 1
    pinMode(BUTTON2_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 2
    pinMode(BUTTON3_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 3
    pinMode(BUTTON4_PIN, INPUT_PULLUP); // Thiết lập chân cho nút nhấn 4

    task1Mailbox = xQueueCreate(1, sizeof(uint8_t)); // Tạo hàng đợi cho task1
    task2Queue = xQueueCreate(2, sizeof(uint8_t)); // Tạo hàng đợi cho task2
    task3Queue = xQueueCreate(2, sizeof(uint8_t)); // Tạo hàng đợi cho task3

    ws2812Mutex = xSemaphoreCreateMutex(); // Tạo semaphore đồng bộ hóa việc truy cập tài nguyên vào WS2812
    mutexHandle = xSemaphoreCreateMutex(); // Bảo vệ biến `taskSelect` khi có sự thay đổi giữa các task trong task4

    xTaskCreate(task1Sender, "Task1Sender", 2048, NULL, 1, NULL); // Tạo task1Sender xử lý task 1
    xTaskCreate(task1Receiver, "Task1Receiver", 2048, NULL, 1, NULL); // Tạo task1Receiver xử lý task 1
    xTaskCreate(task2, "Task2", 2048, NULL, 1, NULL); // Tạo task2 xử lý task 2
    xTaskCreate(task3, "Task3", 2048, NULL, 1, NULL); // Tạo task3 xử lý task 3
    xTaskCreate(task4_1, "Task4_1", 2048, NULL, 1, NULL); // Tạo task4_1 xử lý task 4
    xTaskCreate(task4_2, "Task4_2", 2048, NULL, 1, NULL); // Tạo task4_2 xử lý task 4
    xTaskCreate(switchTask4, "SwitchTask4", 2048, NULL, 1, NULL); // Tạo switchTask4 xử lý task 4
   
    Serial.begin(9600); // Khởi động giao tiếp Serial
}

void loop() {
    static uint8_t lastButton1State = HIGH;
    static uint8_t lastButton2State = HIGH;
    static uint8_t lastButton3State = HIGH;
    static uint8_t lastButton4State = HIGH;

    uint8_t currentButton1State = digitalRead(BUTTON1_PIN);
    uint8_t currentButton2State = digitalRead(BUTTON2_PIN);
    uint8_t currentButton3State = digitalRead(BUTTON3_PIN);
    uint8_t currentButton4State = digitalRead(BUTTON4_PIN);
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
