1. TỔNG QUAN HỆ THỐNG
Hệ thống này được xây dựng dựa trên nền tảng ESP32, sử dụng để giám sát và điều khiển các thiết bị thông qua giao thức ZigBee và Firebase. Hệ thống cho phép đọc dữ liệu từ các thiết bị slave (SLV1 và SLV2), hiển thị thông tin lên màn hình LCD ILI9341 và đồng bộ dữ liệu với Firebase.

1.1. Các chức năng chính
- Kết nối với WiFi và Firebase
- Giao tiếp với các thiết bị slave qua ZigBee
- Hiển thị dữ liệu lên màn hình LCD
- Điều khiển LED (bật/tắt)
- Tự động thực hiện các tác vụ theo lịch trình hàng ngày
- Đồng bộ thời gian qua NTP
- Gửi dữ liệu lên Firebase và đọc dữ liệu từ Firebase

1.2. Cấu trúc phần cứng
- ESP32: Vi điều khiển chính
- Màn hình ILI9341: Hiển thị dữ liệu
- Module ZigBee: Giao tiếp với các thiết bị slave
- LED: Đèn báo trạng thái
- Nút nhấn: Điều khiển thủ công

2. CẤU TRÚC TỔ CHỨC MÃ NGUỒN
2.1. Các file chính
- CONFIG.h / CONFIG.cpp: Định nghĩa cấu trúc dữ liệu và các hằng số cấu hình
- FUNCTION.h / FUNCTION.cpp: Chứa các hàm xử lý chức năng
- main.ino: Chương trình chính

 2.2. Các kiểu dữ liệu chính
 Trong CONFIG.h:
- lcd_pins: Cấu hình chân kết nối màn hình LCD
- peripheral__pin: Cấu hình chân kết nối các thiết bị ngoại vi
- timing_variables: Quản lý thời gian và tần suất thực hiện các tác vụ
- last_data_value: Lưu trữ giá trị mới nhất từ các cảm biến
- DAILY_TASK: Lịch trình tự động thực hiện hàng ngày
- status_var: Lưu trạng thái các thiết bị
- buffer_t: Bộ đệm để xử lý dữ liệu nhận được

3. CHI TIẾT CÁC MODULE VÀ CHỨC NĂNG
3.1. Khởi tạo và cấu hình (setup)
3.1.1. Khởi tạo phần cứng:
void peripheral_init(peripheral pin){
  pin->ZIGBEE_RX = 16;
  pin->ZIGBEE_TX = 17;
  pin->LED_PIN = 5;
  pin->BUTTON_PIN = 32;
}

3.1.2. Khởi tạo LCD (main):
void in_text_ra_lcd(){
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
}

3.1.3. Kết nối WiFi (main):
WiFi.begin(WIFI_SSID, WIFI_PASS);
// Chờ kết nối...

3.1.4. Khởi tạo Firebase (main):
config.api_key = API_KEY;
config.database_url = DATABASE_URL;
// Đăng ký và kết nối...

3.2. Xử lý dữ liệu từ ZigBee
3.2.1. Polling các thiết bị slave (FUNCTION.cpp):
void poll_id(uint8_t &slave_id, buffer_t &buffer) {
    // Luân phiên gửi lệnh đến SLV_01 và SLV_02
}

3.2.2. Nhận và xử lý dữ liệu (main):
void nhan_data(buffer_t &buffer) {
    // Đọc dữ liệu từ cổng serial và xử lý
    // Phát hiện và xử lý frame dữ liệu
}

void processSLV1Data(const char s) {
    // Phân tích dữ liệu từ SLV1
    // Cập nhật biến lưu trữ và hiển thị lên LCD
}

void processSLV2Data(const char s) {
    // Phân tích dữ liệu từ SLV2
    // Cập nhật biến lưu trữ và hiển thị lên LCD
}

3.3. Tương tác với Firebase
3.3.1. Gửi dữ liệu lên Firebase (FUNCTION.cpp):
void send_value_to_firebase(timing_variables timing, last_data_value data) {
    // Gửi dữ liệu từ các cảm biến lên Firebase theo chu kỳ
}

void send_state_to_firebase(status_var status) {
    // Gửi trạng thái LED lên Firebase
}

3.3.2. Đọc dữ liệu từ Firebase (FUNCTION.cpp):
void readFirebaseData(peripheral pin, status_var status) {
    // Đọc trạng thái LED từ Firebase và cập nhật
}

void readDailyTaskSchedule(DAILY_TASK &daily_task) {
    // Đọc lịch trình tự động từ Firebase
}

3.4. Xử lý thời gian và tác vụ tự động
3.4.1. Đồng bộ thời gian (main):
void sync_time() {
    // Đồng bộ thời gian qua NTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    // Thiết lập múi giờ
}

3.4.2. Thực hiện tác vụ theo lịch trình:
// Trong hàm loop():
if (timeinfo.tm_hour == daily_task.taskHour_on && 
    timeinfo.tm_min == daily_task.taskMinute_on && 
    status.last_run_day_on != timeinfo.tm_mday) {
    daily_task_test_on();  
    status.last_run_day_on = timeinfo.tm_mday;  
}

3.5. Xử lý ngắt (Interrupt), (main)
void IRAM_ATTR nhan_nut(){
  // Xử lý chống dội
  // Đảo trạng thái LED khi nhấn nút
}

4. LUỒNG XỬ LÝ CHÍNH
4.1. Khởi động hệ thống
- Khởi tạo Serial và ngoại vi
- Cấu hình GPIO (LED, nút nhấn)
- Kết nối WiFi
- Đồng bộ thời gian qua NTP
- Kết nối và đăng ký với Firebase
- Đọc cấu hình lịch trình từ Firebase
- Khởi tạo màn hình LCD
- Khởi tạo giao tiếp ZigBee

4.2. Vòng lặp chính (loop)
- Gửi lệnh poll đến các thiết bị slave luân phiên
- Nhận và xử lý dữ liệu từ các thiết bị slave
- Đọc trạng thái từ Firebase
- Gửi dữ liệu cảm biến lên Firebase theo chu kỳ
- Cập nhật trạng thái lên Firebase khi có thay đổi
- Kiểm tra và thực hiện tác vụ tự động theo lịch trình

5. CẤU HÌNH VÀ THIẾT LẬP
5.1. Thiết lập WiFi
define WIFI_SSID "Been"
define WIFI_PASS "11119999"

5.2. Thiết lập Firebase
define API_KEY "AIzaSyCNLfTrT6w3K2ipz9DBT198YocfGbZarII"
define DATABASE_URL "https://nckh-dd303-default-rtdb.firebaseio.com/"

5.3. Cấu trúc dữ liệu trên Firebase
- Sensor/
  - WTR_data1: Giá trị cảm biến nước của Slave 1
  - PH_data1: Giá trị pH của Slave 1
  - TDS_data1: Giá trị TDS của Slave 1
  - WTR_data2: Giá trị cảm biến nước của Slave 2
  - PH_data2: Giá trị pH của Slave 2
  - TDS_data2: Giá trị TDS của Slave 2
  - LED: Trạng thái LED (0/1)

- Daily_Task/
  - Hour_on: Giờ bật
  - Minutes_on: Phút bật
  - Hour_off: Giờ tắt
  - Minutes_off: Phút tắt

5.4. Định dạng dữ liệu từ các thiết bị slave
- Slave 1: `<S1,WTR=xx.x,PH=xx.x,TDS=xx.x>`
- Slave 2: `<S2,WTR=xx.x,PH=xx.x,TDS=xx.x>`

6. HƯỚNG DẪN SỬ DỤNG VÀ BẢO TRÌ
6.1. Cập nhật cấu hình
- Thay đổi thông tin WiFi và Firebase trong mã nguồn
- Điều chỉnh chân kết nối trong `peripheral_init()`
- Thay đổi lịch trình trong Firebase (Daily_Task/)

6.2. Bảo trì và xử lý sự cố
6.2.1. Không kết nối được WiFi:
- Kiểm tra SSID và mật khẩu
- Đảm bảo ESP32 trong phạm vi WiFi

6.2.2. Không kết nối được Firebase:
- Kiểm tra API_KEY và DATABASE_URL
- Thử gọi hàm `reconnect_firebase()`

6.2.3. Không nhận được dữ liệu từ slave:
- Kiểm tra kết nối ZigBee
- Đảm bảo các thiết bị slave đang hoạt động
- Kiểm tra định dạng dữ liệu từ slave

6.2.4. Không hiển thị lên LCD:
- Kiểm tra kết nối SPI
- Kiểm tra nguồn điện cung cấp cho LCD

6.3. Mở rộng hệ thống
6.3.1. Thêm thiết bị slave:
- Cập nhật hàm `poll_id()` để thêm ID slave mới
- Tạo hàm xử lý tương ứng (ví dụ: `processSLV3Data()`)
- Thêm cấu trúc dữ liệu trong Firebase

6.3.2. Thêm cảm biến mới:
- Cập nhật cấu trúc `last_data_value` trong CONFIG.h
- Cập nhật định dạng dữ liệu từ slave
- Cập nhật hàm xử lý dữ liệu và hiển thị

7. KẾT LUẬN
Hệ thống đã được thiết kế để có thể mở rộng và linh hoạt trong việc giám sát và điều khiển. Mã nguồn được tổ chức theo module để dễ dàng bảo trì và nâng cấp. Bằng cách tuân theo tài liệu này, việc hiểu và quản lý mã nguồn sẽ trở nên dễ dàng hơn, đặc biệt là đối với những người có trí nhớ kém, như chính tôi :).