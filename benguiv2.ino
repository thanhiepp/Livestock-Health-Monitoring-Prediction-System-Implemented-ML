#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

// ==========================================
// CẤU HÌNH HỆ THỐNG - NODE 2
// ==========================================
#define NODE_ID 2                    // ID của người dùng 2
#define SEND_INTERVAL 5000           // Gửi mỗi 5 giây
#define MAGIC_BYTE 0xA2              // Node 2 (khác với Node 1: 0xA1)

// GPS cố định - Đà Nẵng
#define FIXED_LATITUDE  15.97458646752691     
#define FIXED_LONGITUDE 108.25235621042664  
#define USE_FIXED_GPS   true  // true = dùng GPS cố định, false = dùng GPS thực

// ==========================================
// CẤU HÌNH ĐỘ ẨM RANDOM (Giả lập DHT22)
// ==========================================
#define HUMIDITY_MIN 50.0        // Độ ẩm tối thiểu (%)
#define HUMIDITY_MAX 85.0        // Độ ẩm tối đa (%)
#define HUMIDITY_VARIATION 5.0   // Biến thiên mỗi lần đọc (%)

// ==========================================
// NGƯỠNG Y TẾ
// ==========================================
#define HUMAN_TEMP_MIN 25.0
#define HUMAN_TEMP_MAX 42.0
#define FEVER_THRESHOLD 37.5
#define LOW_TEMP_THRESHOLD 36.0

// ==========================================
// CHÂN KẾT NỐI ESP32
// ==========================================
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define GPS_RX 16
#define GPS_TX 17

// LoRa Configuration
#define LORA_FREQ 433E6
#define LORA_SF 10
#define LORA_BW 250E3
#define LORA_CR 5
#define LORA_SW 0x12
#define LORA_POWER 20

// ==========================================
// STRUCT BINARY PACKET - 27 bytes
// ==========================================
struct __attribute__((packed)) LoRaPacket {
  uint8_t  magic;        // 1 byte: 0xA2 cho Node 2
  uint16_t counter;      // 2 bytes: packet counter
  int16_t  temp;         // 2 bytes: nhiệt độ môi trường x10 (MLX ambient)
  uint16_t humid;        // 2 bytes: độ ẩm x10 (RANDOM - giả lập DHT22)
  int16_t  tempDS;       // 2 bytes: thân nhiệt x10 (MLX object temp)
  int32_t  lat;          // 4 bytes: vĩ độ x1000000
  int32_t  lon;          // 4 bytes: kinh độ x1000000

  // Accelerometer (Node 2 không có MPU → 0)
  int16_t  ax;           // 2 bytes
  int16_t  ay;           // 2 bytes
  int16_t  az;           // 2 bytes

  // Gyroscope (Node 2 không có MPU → 0)
  int16_t  gx;           // 2 bytes
  int16_t  gy;           // 2 bytes
  int16_t  gz;           // 2 bytes

  uint8_t  hr;           // 1 byte: nhịp tim (không có sensor → 0)
  int8_t   rssi;         // 1 byte: RSSI placeholder
};

// ==========================================
// KHỞI TẠO ĐỐI TƯỢNG
// ==========================================
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // UART1 cho GPS
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// ==========================================
// BIẾN TRẠNG THÁI
// ==========================================
bool mlxAvailable = false;
bool gpsReady = false;

// ==========================================
// BỘ LỌC NHIỆT ĐỘ - MEDIAN FILTER
// ==========================================
#define TEMP_FILTER_SIZE 5
float tempAmbientBuffer[TEMP_FILTER_SIZE];   // Nhiệt độ môi trường
float tempObjectBuffer[TEMP_FILTER_SIZE];    // Thân nhiệt
int tempAmbientIndex = 0;
int tempObjectIndex = 0;
bool tempAmbientFilled = false;
bool tempObjectFilled = false;

// ==========================================
// ĐỘ ẨM RANDOM - SMOOTH VARIATION
// ==========================================
float currentHumidity = 65.0;  // Giá trị khởi tạo (%)
float targetHumidity = 65.0;   // Mục tiêu để smooth transition

// ==========================================
// TIMING VÀ COUNTER
// ==========================================
unsigned long lastSend = 0;
uint16_t packetCounter = 0;
unsigned long startTime = 0;

// ==========================================
// KHAI BÁO HÀM TRƯỚC
// ==========================================
void printHeader();
void initLoRa();
void initMLX90614();
void initGPS();
void processGPS();
float readAmbientTemperature();
float readObjectTemperature();
float readHumidity();
float getMedianFromBuffer(float* buffer, int size);
void collectAndSendData();
void sendBinaryPacket(float envTemp, float bodyTemp, float humidity, double lat, double lon);
void printSensorData(float envTemp, float bodyTemp, float humidity, double lat, double lon);
void printFloat(float value, int decimals, const char* unit);

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  startTime = millis();
  
  // Khởi tạo random seed từ analog pin
  randomSeed(analogRead(0));
  
  printHeader();
  
  // Khởi tạo buffer với NAN
  for (int i = 0; i < TEMP_FILTER_SIZE; i++) {
    tempAmbientBuffer[i] = NAN;
    tempObjectBuffer[i] = NAN;
  }
  
  // Khởi tạo độ ẩm random ban đầu
  currentHumidity = random(HUMIDITY_MIN * 10, HUMIDITY_MAX * 10) / 10.0;
  targetHumidity = currentHumidity;
  
  // Khởi tạo I2C
  Wire.begin();
  delay(100);
  
  // Khởi tạo các module
  initLoRa();
  initMLX90614();
  initGPS();
  
  Serial.println(F("\n[READY] Node 2 Active - ESP32 LoRa"));
  Serial.println(F("[MODE] Continuous Send - MLX90614 + GPS + RANDOM HUMIDITY"));
  Serial.println(F("=====================================\n"));
  
  delay(1000);
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  processGPS();
  
  if (millis() - lastSend >= SEND_INTERVAL) {
    collectAndSendData();
    lastSend = millis();
  }
  
  delay(10); // Small delay to prevent watchdog
}

// ==========================================
// XỬ LÝ GPS
// ==========================================
void processGPS() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
    
    if (gps.location.isUpdated() && !gpsReady) {
      gpsReady = true;
      Serial.println(F("[GPS] First fix acquired!"));
    }
  }
}

// ==========================================
// ĐỌC NHIỆT ĐỘ MÔI TRƯỜNG (MLX90614 Ambient)
// ==========================================
float readAmbientTemperature() {
  if (!mlxAvailable) return NAN;
  
  float raw = mlx.readAmbientTempC();
  
  if (isnan(raw) || raw < -40 || raw > 80) {
    int usedSize = tempAmbientFilled ? TEMP_FILTER_SIZE : tempAmbientIndex;
    if (usedSize > 0) {
      return getMedianFromBuffer(tempAmbientBuffer, usedSize);
    }
    return NAN;
  }
  
  tempAmbientBuffer[tempAmbientIndex] = raw;
  tempAmbientIndex = (tempAmbientIndex + 1) % TEMP_FILTER_SIZE;
  if (tempAmbientIndex == 0) tempAmbientFilled = true;
  
  int usedSize = tempAmbientFilled ? TEMP_FILTER_SIZE : tempAmbientIndex;
  return getMedianFromBuffer(tempAmbientBuffer, usedSize);
}

// ==========================================
// ĐỌC THÂN NHIỆT (MLX90614 Object)
// ==========================================
float readObjectTemperature() {
  if (!mlxAvailable) return NAN;
  
  float raw = mlx.readObjectTempC();
  
  if (isnan(raw) || raw < HUMAN_TEMP_MIN || raw > HUMAN_TEMP_MAX) {
    int usedSize = tempObjectFilled ? TEMP_FILTER_SIZE : tempObjectIndex;
    if (usedSize > 0) {
      float median = getMedianFromBuffer(tempObjectBuffer, usedSize);
      if (!isnan(median)) return median;
    }
    return NAN;
  }
  
  tempObjectBuffer[tempObjectIndex] = raw;
  tempObjectIndex = (tempObjectIndex + 1) % TEMP_FILTER_SIZE;
  if (tempObjectIndex == 0) tempObjectFilled = true;
  
  int usedSize = tempObjectFilled ? TEMP_FILTER_SIZE : tempObjectIndex;
  return getMedianFromBuffer(tempObjectBuffer, usedSize);
}

// ==========================================
// ĐỌC ĐỘ ẨM (RANDOM - GIẢ LẬP DHT22)
// ==========================================
float readHumidity() {
  // Tạo biến thiên nhỏ mỗi lần đọc để giá trị thay đổi tự nhiên
  // Random walk algorithm cho realistic simulation
  
  // 70% chance giữ nguyên target, 30% chance thay đổi
  if (random(0, 100) < 30) {
    // Tạo target mới trong khoảng ±HUMIDITY_VARIATION
    float change = random(-HUMIDITY_VARIATION * 10, HUMIDITY_VARIATION * 10) / 10.0;
    targetHumidity = currentHumidity + change;
    
    // Giới hạn trong range cho phép
    if (targetHumidity < HUMIDITY_MIN) targetHumidity = HUMIDITY_MIN;
    if (targetHumidity > HUMIDITY_MAX) targetHumidity = HUMIDITY_MAX;
  }
  
  // Smooth transition về target (tạo cảm giác tự nhiên)
  float diff = targetHumidity - currentHumidity;
  currentHumidity += diff * 0.3; // 30% của khoảng cách mỗi lần
  
  // Thêm noise nhỏ để giống sensor thật (±0.5%)
  float noise = random(-5, 6) / 10.0;
  float finalHumidity = currentHumidity + noise;
  
  // Đảm bảo trong range
  if (finalHumidity < HUMIDITY_MIN) finalHumidity = HUMIDITY_MIN;
  if (finalHumidity > HUMIDITY_MAX) finalHumidity = HUMIDITY_MAX;
  
  return finalHumidity;
}

// ==========================================
// HÀM TÍNH MEDIAN (Bộ Lọc)
// ==========================================
float getMedianFromBuffer(float* buffer, int size) {
  if (size <= 0) return NAN;
  
  float sorted[TEMP_FILTER_SIZE];
  int validCount = 0;
  
  for (int i = 0; i < size; i++) {
    if (!isnan(buffer[i])) {
      sorted[validCount++] = buffer[i];
    }
  }
  
  if (validCount == 0) return NAN;
  
  // Bubble sort
  for (int i = 0; i < validCount - 1; i++) {
    for (int j = 0; j < validCount - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float tmp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = tmp;
      }
    }
  }
  
  return sorted[validCount / 2];
}

// ==========================================
// THU THẬP VÀ GỬI DỮ LIỆU
// ==========================================
void collectAndSendData() {
  unsigned long currentTime = millis() - startTime;
  unsigned long seconds = currentTime / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  Serial.println(F("\n====================================="));
  Serial.print(F("  NODE 2 - PACKET #"));
  Serial.println(packetCounter);
  Serial.print(F("  TIME: "));
  Serial.print(hours % 24);
  Serial.print(F(":"));
  if ((minutes % 60) < 10) Serial.print(F("0"));
  Serial.print(minutes % 60);
  Serial.print(F(":"));
  if ((seconds % 60) < 10) Serial.print(F("0"));
  Serial.print(seconds % 60);
  Serial.print(F(" ("));
  Serial.print(currentTime);
  Serial.println(F(" ms)"));
  Serial.println(F("====================================="));
  
  // Đọc cảm biến
  float envTemp = readAmbientTemperature();
  float bodyTemp = readObjectTemperature();
  float humidity = readHumidity();  // ✅ ĐỌC ĐỘ ẨM RANDOM
  
  // Xử lý GPS - Sử dụng GPS cố định Đà Nẵng
  double currentLat = FIXED_LATITUDE;
  double currentLon = FIXED_LONGITUDE;
  
  // In dữ liệu
  printSensorData(envTemp, bodyTemp, humidity, currentLat, currentLon);
  
  // Gửi packet
  sendBinaryPacket(envTemp, bodyTemp, humidity, currentLat, currentLon);
  
  Serial.print(F("[INFO] Sent at: "));
  Serial.print(hours % 24);
  Serial.print(F(":"));
  if ((minutes % 60) < 10) Serial.print(F("0"));
  Serial.print(minutes % 60);
  Serial.print(F(":"));
  if ((seconds % 60) < 10) Serial.print(F("0"));
  Serial.println(seconds % 60);
  
  Serial.println(F("=====================================\n"));
}

// ==========================================
// IN DỮ LIỆU CẢM BIẾN
// ==========================================
void printSensorData(float envTemp, float bodyTemp, float humidity, double lat, double lon) {
  Serial.print(F("ENV  Temp: "));
  printFloat(envTemp, 1, "C");
  Serial.println();
  
  Serial.print(F("HUMIDITY : "));  // ✅ HIỂN THỊ ĐỘ ẨM
  printFloat(humidity, 1, "%");
  Serial.print(F(" (SIMULATED)"));  // Đánh dấu là giả lập
  Serial.println();
  
  Serial.print(F("BODY Temp: "));
  if (!isnan(bodyTemp)) {
    Serial.print(bodyTemp, 2);
    Serial.print(F(" C"));
    
    if (bodyTemp > FEVER_THRESHOLD) {
      Serial.print(F(" [WARNING: FEVER]"));
    } else if (bodyTemp < LOW_TEMP_THRESHOLD) {
      Serial.print(F(" [WARNING: LOW]"));
    } else {
      Serial.print(F(" [NORMAL]"));
    }
  } else {
    Serial.print(F("N/A"));
  }
  Serial.println();
  
  Serial.print(F("GPS: FIXED ("));
  Serial.print(lat, 6);
  Serial.print(F(", "));
  Serial.print(lon, 6);
  Serial.println(F(") - Da Nang"));
}

void printFloat(float value, int decimals, const char* unit) {
  if (isnan(value)) {
    Serial.print(F("N/A"));
  } else {
    Serial.print(value, decimals);
    Serial.print(" ");
    Serial.print(unit);
  }
}

// ==========================================
// GỬI BINARY PACKET
// ==========================================
void sendBinaryPacket(float envTemp, float bodyTemp, float humidity, double lat, double lon) {
  LoRaPacket pkt;
  
  pkt.magic = MAGIC_BYTE;
  pkt.counter = packetCounter++;
  
  // Nhiệt độ
  pkt.temp = isnan(envTemp) ? -9999 : (int16_t)(envTemp * 10);
  pkt.humid = isnan(humidity) ? 0 : (uint16_t)(humidity * 10);  // ✅ GỬI ĐỘ ẨM RANDOM
  pkt.tempDS = isnan(bodyTemp) ? -9999 : (int16_t)(bodyTemp * 10);
  
  // GPS - Cố định Đà Nẵng
  pkt.lat = (int32_t)(lat * 1000000);
  pkt.lon = (int32_t)(lon * 1000000);
  
  // Accelerometer & Gyroscope (Node 2 không có → 0)
  pkt.ax = 0;
  pkt.ay = 0;
  pkt.az = 0;
  pkt.gx = 0;
  pkt.gy = 0;
  pkt.gz = 0;
  
  // Heart rate (Node 2 không có → 0)
  pkt.hr = 0;
  pkt.rssi = 0;
  
  const size_t pktSize = sizeof(pkt);
  uint8_t buf[32];
  memcpy(buf, &pkt, pktSize);
  
  Serial.print(F("\n>>> SENDING: "));
  Serial.print(pktSize);
  Serial.println(F(" bytes"));
  
  Serial.print(F("HEX: "));
  for (size_t i = 0; i < pktSize; i++) {
    if (buf[i] < 16) Serial.print('0');
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  
  LoRa.beginPacket();
  LoRa.write(buf, pktSize);
  LoRa.endPacket();
  
  Serial.println(F("[LoRa] Packet sent successfully"));
}

// ==========================================
// KHỞI TẠO LORA
// ==========================================
void initLoRa() {
  Serial.print(F("[LoRa] Initializing... "));
  
  SPI.begin();
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println(F("FAILED!"));
    while (1) {
      delay(1000);
    }
  }
  
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SW);
  LoRa.setTxPower(LORA_POWER);
  
  Serial.print(F("OK (SF"));
  Serial.print(LORA_SF);
  Serial.print(F(", BW"));
  Serial.print((int)(LORA_BW / 1000));
  Serial.print(F("kHz, "));
  Serial.print(LORA_POWER);
  Serial.println(F("dBm)"));
}

// ==========================================
// KHỞI TẠO MLX90614
// ==========================================
void initMLX90614() {
  Serial.print(F("[MLX90614] "));
  
  if (mlx.begin()) {
    mlxAvailable = true;
    Serial.println(F("OK"));
    
    // Đọc thử
    delay(100);
    float ambient = mlx.readAmbientTempC();
    float object = mlx.readObjectTempC();
    
    Serial.print(F("  Ambient: "));
    Serial.print(ambient, 1);
    Serial.print(F(" C, Object: "));
    Serial.print(object, 1);
    Serial.println(F(" C"));
  } else {
    mlxAvailable = false;
    Serial.println(F("NOT FOUND"));
  }
}

// ==========================================
// KHỞI TẠO GPS
// ==========================================
void initGPS() {
  Serial.print(F("[GPS] "));
  Serial.println(F("Using FIXED coordinates - Da Nang"));
  Serial.print(F("  Lat: "));
  Serial.println(FIXED_LATITUDE, 8);
  Serial.print(F("  Lon: "));
  Serial.println(FIXED_LONGITUDE, 8);
  gpsReady = true;
}

// ==========================================
// HEADER
// ==========================================
void printHeader() {
  Serial.println(F("\n====================================="));
  Serial.println(F("  NODE 2 - ESP32 LORA MONITOR"));
  Serial.println(F("  MLX90614 + GPS + RANDOM HUMIDITY"));
  Serial.println(F("  CONTINUOUS SEND MODE"));
  Serial.println(F("====================================="));
  Serial.print(F("  Node ID: "));
  Serial.println(NODE_ID);
  Serial.print(F("  Magic: 0x"));
  Serial.println(MAGIC_BYTE, HEX);
  Serial.print(F("  Packet Size: "));
  Serial.print(sizeof(LoRaPacket));
  Serial.println(F(" bytes"));
  Serial.print(F("  Send Interval: "));
  Serial.print(SEND_INTERVAL / 1000);
  Serial.println(F("s"));
  Serial.print(F("  LoRa Freq: "));
  Serial.print((int)(LORA_FREQ / 1E6));
  Serial.println(F(" MHz"));
  Serial.print(F("  Humidity Range: "));
  Serial.print(HUMIDITY_MIN, 0);
  Serial.print(F("% - "));
  Serial.print(HUMIDITY_MAX, 0);
  Serial.println(F("%"));
  Serial.println(F("=====================================\n"));
}