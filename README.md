# FreeRTOS-Multitasking

## Hệ Thống Điều Khiển LED Ứng Dụng FreeRTOS

## Giới Thiệu
Dự án này nhằm mục đích xây dựng một hệ thống điều khiển đa dạng các loại LED sử dụng hệ điều hành thời gian thực FreeRTOS. Hệ thống này có khả năng điều khiển từ LED đơn giản đến các module LED phức tạp, cung cấp một giải pháp linh hoạt và mạnh mẽ cho các ứng dụng cần hiển thị đèn LED đa dạng.

## Các Module LED
- Module hiển thị 4 led 7 đoạn
- Module LED RGB 3 màu
- Module 8 Led RGB WS2812
- Module 8 LED 5050 RGB WS2812B
- Module 4 Led RGB WS2812
- Màn hình Oled 0.91inch
- Mạch hiển thị led ma trận MAX7219
- Led lùn 5MM nhấp nháy 7 màu
- Led 1W WW
- LED đơn

## Công Nghệ và Kiến Thức Áp Dụng
- **Semaphore**: Đảm bảo quản lý tài nguyên một cách hiệu quả.
  - **Counting Semaphore**: Cho phép nhiều hơn một task có thể truy cập vào tài nguyên.
  - **Binary Semaphore**: Hoạt động như một mutex với quyền sở hữu không được quản lý.
- **Software Timer**: Cung cấp khả năng thực hiện các hành động sau một khoảng thời gian nhất định.
- **Queue**:
  - **Message Queue**: Gửi và nhận các thông điệp giữa các task.
  - **Mail Queue**: Gửi các cấu trúc dữ liệu phức tạp hơn.
- **Mailbox**: Gửi thông điệp nhanh chóng giữa các task.
- **Interrupt**: Phản ứng nhanh với các sự kiện bên ngoài.

## Cấu Trúc Thư Mục
Dự án này bao gồm các thư mục sau:
- `MainCode`: Chứa mã nguồn chính của dự án.
- `libraries`: Chứa các thư viện cần thiết cho dự án.

## Cài đặt
Để cài đặt và chạy dự án này, bạn cần có môi trường phát triển FreeRTOS và Arduino IDE. Bạn có thể tải xuống mã nguồn dự án từ đây và mở nó trong môi trường phát triển của mình.

1. **Cài Đặt Arduino IDE**: Tải và cài đặt Arduino IDE từ trang web chính thức của Arduino.
2. **Clone Dự Án Từ GitHub**: Mở Command Prompt hoặc Terminal và chạy lệnh sau để clone dự án:
   ```
    git clone https://github.com/minzdat/FreeRTOS-Multitasking.git
   ```
4. **Tải và Cài Đặt Thư Viện**: Sao chép thư mục `libraries` vào thư mục `libraries` của Arduino IDE trên máy tính của bạn.
5. **Mở Dự Án**: Mở Arduino IDE, chọn `File -> Open...` và điều hướng đến thư mục `MainCode` của dự án.
6. **Biên Dịch và Tải Lên**: Nhấn nút `Upload` để biên dịch và tải mã nguồn lên bo mạch Arduino của bạn.
   
## Đóng góp
Chúng tôi trân trọng mọi sự đóng góp từ các thành viên trong cộng đồng.
