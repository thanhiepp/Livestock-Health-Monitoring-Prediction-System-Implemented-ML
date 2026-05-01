#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <MPU6050.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// Activity classification moved to Gateway (ESP32)
// Node only sends raw sensor data

// ==========================================
// CẤU HÌNH HỆ THỐNG - HUMAN ACTIVITY MONITOR
// ==========================================
#define NODE_ID 1                    // ID của người dùng (1 hoặc 2)
#define SEND_INTERVAL 5000           // Gửi mỗi 5 giây (nhanh hơn để theo dõi hoạt động)
#define MAGIC_BYTE 0xA1              // Node 1

#define FIXED_LATITUDE  15.97466418620254     
#define FIXED_LONGITUDE 108.25227581750812  
#define USE_FIXED_GPS   true  
// ==========================================
// NGƯỠNG Y TẾ CON NGƯỜI
// ==========================================
// Thân nhiệt bình thường: 36.5 - 37.5°C
// Nhịp tim bình thường (người lớn): 60 - 100 BPM
#define HUMAN_TEMP_MIN 25.0          // Nhiệt độ tối thiểu hợp lệ
#define HUMAN_TEMP_MAX 42.0          // Nhiệt độ tối đa hợp lệ
#define HUMAN_HR_MIN 40              // Nhịp tim tối thiểu
#define HUMAN_HR_MAX 200             // Nhịp tim tối đa

// ==========================================
// CHÂN KẾT NỐI
// ==========================================
#define LORA_SS 10
#define LORA_RST 9
#define LORA_DIO0 2
#define GPS_RX 5
#define GPS_TX 6
#define DHT_PIN 7
#define DHT_TYPE DHT22
#define DS18B20_PIN 4

// ==========================================
// STRUCT BINARY PACKET (27 bytes) - Raw sensor data
// ==========================================
struct __attribute__((packed)) LoRaPacket {
  uint8_t  magic;        // 1 byte: Magic byte
  uint16_t counter;      // 2 bytes: packet counter
  int16_t  temp;         // 2 bytes: nhiệt độ môi trường x10
  uint16_t humid;        // 2 bytes: độ ẩm x10
  int16_t  tempDS;       // 2 bytes: thân nhiệt x10 (DS18B20)
  int32_t  lat;          // 4 bytes: vĩ độ x1000000
  int32_t  lon;          // 4 bytes: kinh độ x1000000

  // MPU6050 - Accelerometer
  int16_t  ax;           // 2 bytes
  int16_t  ay;           // 2 bytes
  int16_t  az;           // 2 bytes

  // MPU6050 - Gyroscope (for Gateway TinyML)
  int16_t  gx;           // 2 bytes
  int16_t  gy;           // 2 bytes
  int16_t  gz;           // 2 bytes

  uint8_t  hr;           // 1 byte: nhịp tim
  int8_t   rssi;         // 1 byte: RSSI (placeholder)
};

// ==========================================
// KHỞI TẠO ĐỐI TƯỢNG
// ==========================================
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
DHT dht(DHT_PIN, DHT_TYPE);
MPU6050 mpu(0x68);
MAX30105 particleSensor;
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

// ==========================================
// BIẾN TRẠNG THÁI CẢM BIẾN
// ==========================================
bool mpuAvailable = false;
bool maxAvailable = false;
bool ds18b20Available = false;

// ==========================================
// BỘ LỌC NHIỆT ĐỘ - MEDIAN FILTER
// ==========================================
#define TEMP_FILTER_SIZE 5
float tempBuffer[TEMP_FILTER_SIZE];   // môi trường
float tempDSBuffer[TEMP_FILTER_SIZE]; // thân nhiệt
int tempBufferIndex = 0;
int tempDSBufferIndex = 0;
bool tempBufferFilled = false;
bool tempDSBufferFilled = false;

// ==========================================
// MAX30102 - NHỊP TIM
// ==========================================
#define HR_BUFFER_SIZE 8
byte hrRates[HR_BUFFER_SIZE];
byte hrRateSpot = 0;
long hrLastBeat = 0;
int  hrAvg = 0;
int  hrValidReadings = 0;
bool hrFingerDetected = false;
unsigned long hrLastFingerTime = 0;
bool   hrStable      = false;  // đã có nhịp ổn định chưa
int    hrStableAvg   = 0;      // nhịp tim đã được làm mượt để in báo cáo
int    hrLastRawBpm  = 0;      // BPM cuối cùng (chưa lọc)


// ==========================================
// MPU6050 - GIA TỐC KẾ VÀ CON QUAY HỒI CHUYỂN
// ==========================================
#define MPU_BUFFER_SIZE 20  // Tăng buffer để phát hiện pattern tốt hơn
int16_t axBuffer[MPU_BUFFER_SIZE] = {0};
int16_t ayBuffer[MPU_BUFFER_SIZE] = {0};
int16_t azBuffer[MPU_BUFFER_SIZE] = {0};
int16_t gxBuffer[MPU_BUFFER_SIZE] = {0};
int16_t gyBuffer[MPU_BUFFER_SIZE] = {0};
int16_t gzBuffer[MPU_BUFFER_SIZE] = {0};
int mpuBufferIndex = 0;

// Gyro offset calibration
int16_t gxOffset = 0, gyOffset = 0, gzOffset = 0;
bool gyroCalibrated = false;

// Activity metrics
float activityLevel = 0;
float stepCount = 0;
unsigned long lastStepTime = 0;

// ==========================================
// TIMING VÀ COUNTER
// ==========================================
unsigned long lastSend = 0;
uint16_t packetCounter = 0;

// ==========================================
// KHAI BÁO HÀM TRƯỚC
// ==========================================
float getMedianFromBuffer(float* buffer, int size);
void detectSteps();
int lastHeartRate = 0; 

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Wire.begin();
  
  printHeader();

  // Khởi tạo buffer với NAN
  for (int i = 0; i < TEMP_FILTER_SIZE; i++) {
    tempBuffer[i]   = NAN;
    tempDSBuffer[i] = NAN;
  }
  tempBufferIndex   = 0;
  tempDSBufferIndex = 0;
  tempBufferFilled  = false;
  tempDSBufferFilled = false;
  
  // Khởi tạo cảm biến
  initLoRa();
  initDHT();
  initDS18B20();
  scanI2C();
  initMPU6050();
  initMAX30102();
  
  // Calibrate gyro
  if (mpuAvailable) {
    calibrateGyro();
  }
  
  Serial.println(F("\n[READY] Human Activity Monitor Active"));
  Serial.println(F("[MODE] Continuous Send - No change detection"));
  Serial.println(F("=====================================\n"));
  
  delay(1000);
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  processGPS();
  processHeartRate();
  processMPU();
  
  if (millis() - lastSend >= SEND_INTERVAL) {
    collectAndSendData();
    lastSend = millis();
  }
}

// ==========================================
// XỬ LÝ GPS
// ==========================================
void processGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

// ==========================================
// XỬ LÝ NHỊP TIM (MAX30102)
// ==========================================
void processHeartRate() {
  if (!maxAvailable) return;

  long irValue = particleSensor.getIR();

  // Phát hiện có tay
  if (irValue > 30000) {  // có thể giảm ngưỡng từ 50000 xuống 30000 để dễ detect hơn
    if (!hrFingerDetected) {
      // Mới đặt tay vào → reset buffer
      hrFingerDetected   = true;
      hrLastBeat         = millis();
      hrValidReadings    = 0;
      hrRateSpot         = 0;
      hrAvg              = 0;
      hrStable           = false;
      hrStableAvg        = 0;
    }

    if (checkForBeat(irValue)) {
      long delta = millis() - hrLastBeat;
      hrLastBeat = millis();

      float bpm = 60.0f / (delta / 1000.0f);
      hrLastRawBpm = (int)bpm;

      // Chỉ nhận trong khoảng hợp lý
      if (bpm >= HUMAN_HR_MIN && bpm <= HUMAN_HR_MAX) {
        if (hrValidReadings > 0) {
          int diff = abs((int)bpm - hrAvg);
          if (diff > 30) {
            // Nhịp này quá lệch so với trung bình → bỏ qua
            return;
          }
        }

        hrRates[hrRateSpot++] = (byte)bpm;
        if (hrRateSpot >= HR_BUFFER_SIZE) hrRateSpot = 0;
        hrValidReadings++;

        int count = min(hrValidReadings, HR_BUFFER_SIZE);

        // Tính trung bình, min, max trong buffer
        int sum = 0;
        int minB = 255;
        int maxB = 0;
        for (int i = 0; i < count; i++) {
          int v = hrRates[i];
          sum += v;
          if (v < minB) minB = v;
          if (v > maxB) maxB = v;
        }
        hrAvg = sum / count;

        int range = maxB - minB;

        // Điều kiện coi là "ổn định":
        //  - đủ số lần đo
        //  - dải min-max không quá rộng
        const int MIN_BEATS_FOR_STABLE = 5;
        if (count >= MIN_BEATS_FOR_STABLE && range <= 15) {
          hrStable    = true;
          hrStableAvg = hrAvg;
        } else {
          hrStable = false;  // đang trong giai đoạn đo/chưa ổn định
        }
      }
    }
  } else {
    // Không có tay / mất tín hiệu trong 3s → reset
    if (millis() - hrLastBeat > 3000) {
      hrFingerDetected   = false;
      hrValidReadings    = 0;
      hrAvg              = 0;
      hrStable           = false;
      hrStableAvg        = 0;
    }
  }
}

// ==========================================
// XỬ LÝ MPU6050 - PHÁT HIỆN HOẠT ĐỘNG
// ==========================================
void processMPU() {
  if (!mpuAvailable) return;
  
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Apply gyro offset
  gx -= gxOffset;
  gy -= gyOffset;
  gz -= gzOffset;
  
  // Store in circular buffer
  axBuffer[mpuBufferIndex] = ax;
  ayBuffer[mpuBufferIndex] = ay;
  azBuffer[mpuBufferIndex] = az;
  gxBuffer[mpuBufferIndex] = gx;
  gyBuffer[mpuBufferIndex] = gy;
  gzBuffer[mpuBufferIndex] = gz;
  mpuBufferIndex = (mpuBufferIndex + 1) % MPU_BUFFER_SIZE;
  
  // Calculate activity metrics
  calculateActivityLevel();
  detectSteps();
}

// ==========================================
// TÍNH MỨC ĐỘ HOẠT ĐỘNG (for step detection only)
// ==========================================
void calculateActivityLevel() {
  // Calculate variance for activity level estimation
  long axSum = 0, aySum = 0, azSum = 0;
  
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    axSum += axBuffer[i];
    aySum += ayBuffer[i];
    azSum += azBuffer[i];
  }
  
  int16_t axMean = axSum / MPU_BUFFER_SIZE;
  int16_t ayMean = aySum / MPU_BUFFER_SIZE;
  int16_t azMean = azSum / MPU_BUFFER_SIZE;
  
  long variance = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    variance += sq((long)(axBuffer[i] - axMean));
    variance += sq((long)(ayBuffer[i] - ayMean));
    variance += sq((long)(azBuffer[i] - azMean));
  }
  variance /= (MPU_BUFFER_SIZE * 3);
  
  activityLevel = constrain(sqrt((float)variance) / 100.0, 0, 100);
}

// ==========================================
// PHÁT HIỆN BƯỚC CHÂN (STEP DETECTION)
// ==========================================
void detectSteps() {
  // Simple step detection using Z-axis acceleration peaks
  // Only count steps when activity level is moderate to high
  if (activityLevel < 5) {
    return;  // Too little movement for walking
  }
  
  // Get current Z acceleration
  int16_t currentAz = azBuffer[(mpuBufferIndex - 1 + MPU_BUFFER_SIZE) % MPU_BUFFER_SIZE];
  
  // Convert to g
  float az_g = currentAz / 16384.0f;
  
  // Detect peak (step) if acceleration crosses threshold
  static bool wasAbove = false;
  const float STEP_THRESHOLD = 1.2f; // Above 1.2g
  const unsigned long MIN_STEP_INTERVAL = 300; // Min 300ms between steps
  
  if (az_g > STEP_THRESHOLD && !wasAbove) {
    if (millis() - lastStepTime > MIN_STEP_INTERVAL) {
      stepCount++;
      lastStepTime = millis();
    }
    wasAbove = true;
  } else if (az_g < STEP_THRESHOLD) {
    wasAbove = false;
  }
}

// ==========================================
// ĐỌC NHIỆT ĐỘ MÔI TRƯỜNG (DHT22)
// ==========================================
float readTemperatureFiltered() {
  float raw = dht.readTemperature();
  
  if (isnan(raw) || raw < -40 || raw > 80) {
    int usedSize = tempBufferFilled ? TEMP_FILTER_SIZE : tempBufferIndex;
    return getMedianFromBuffer(tempBuffer, usedSize);
  }
  
  tempBuffer[tempBufferIndex] = raw;
  tempBufferIndex = (tempBufferIndex + 1) % TEMP_FILTER_SIZE;
  if (tempBufferIndex == 0) tempBufferFilled = true;
  
  int usedSize = tempBufferFilled ? TEMP_FILTER_SIZE : tempBufferIndex;
  return getMedianFromBuffer(tempBuffer, usedSize);
}

// ==========================================
// ĐỌC THÂN NHIỆT (DS18B20)
// ==========================================
float readBodyTemperature() {
  if (!ds18b20Available) return NAN;
  
  ds18b20.requestTemperatures();
  float raw = ds18b20.getTempCByIndex(0);

  if (raw == DEVICE_DISCONNECTED_C || raw < HUMAN_TEMP_MIN || raw > HUMAN_TEMP_MAX) {
    int usedSize = tempDSBufferFilled ? TEMP_FILTER_SIZE : tempDSBufferIndex;
    if (usedSize > 0) {
      float median = getMedianFromBuffer(tempDSBuffer, usedSize);
      if (!isnan(median)) return median;
    }
    return NAN;
  }
  
  tempDSBuffer[tempDSBufferIndex] = raw;
  tempDSBufferIndex = (tempDSBufferIndex + 1) % TEMP_FILTER_SIZE;
  if (tempDSBufferIndex == 0) tempDSBufferFilled = true;
  
  int usedSize = tempDSBufferFilled ? TEMP_FILTER_SIZE : tempDSBufferIndex;
  return getMedianFromBuffer(tempDSBuffer, usedSize);
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
// LẤY GIA TỐC TRUNG BÌNH
// ==========================================
int16_t getFilteredAccelX() {
  long sum = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    sum += axBuffer[i];
  }
  return sum / MPU_BUFFER_SIZE;
}

int16_t getFilteredAccelY() {
  long sum = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    sum += ayBuffer[i];
  }
  return sum / MPU_BUFFER_SIZE;
}

int16_t getFilteredAccelZ() {
  long sum = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    sum += azBuffer[i];
  }
  return sum / MPU_BUFFER_SIZE;
}

// ==========================================
// LẤY GYRO TRUNG BÌNH (for Gateway TinyML)
// ==========================================
int16_t getFilteredGyroX() {
  long sum = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    sum += gxBuffer[i];
  }
  return sum / MPU_BUFFER_SIZE;
}

int16_t getFilteredGyroY() {
  long sum = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    sum += gyBuffer[i];
  }
  return sum / MPU_BUFFER_SIZE;
}

int16_t getFilteredGyroZ() {
  long sum = 0;
  for (int i = 0; i < MPU_BUFFER_SIZE; i++) {
    sum += gzBuffer[i];
  }
  return sum / MPU_BUFFER_SIZE;
}

// ==========================================
// THU THẬP VÀ GỬI DỮ LIỆU - CONTINUOUS MODE
// ==========================================
void collectAndSendData() {
  // Lấy thời gian hiện tại
  unsigned long currentTime = millis();
  unsigned long seconds = currentTime / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  Serial.println(F("\n====================================="));
  Serial.print(F("  HUMAN ACTIVITY - PACKET #"));
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

  float envTemp = readTemperatureFiltered();
  float humidity = dht.readHumidity();
  float bodyTemp = readBodyTemperature();

  int16_t accelX = mpuAvailable ? getFilteredAccelX() : 0;
  int16_t accelY = mpuAvailable ? getFilteredAccelY() : 0;
  int16_t accelZ = mpuAvailable ? getFilteredAccelZ() : 0;
  int16_t gyroX  = mpuAvailable ? getFilteredGyroX() : 0;
  int16_t gyroY  = mpuAvailable ? getFilteredGyroY() : 0;
  int16_t gyroZ  = mpuAvailable ? getFilteredGyroZ() : 0;

  int heartRate = hrFingerDetected ? hrAvg : 0;

  // Sử dụng GPS cố định hoặc GPS thực tùy cấu hình
  double currentLat, currentLon;
  if (USE_FIXED_GPS) {
    currentLat = FIXED_LATITUDE;
    currentLon = FIXED_LONGITUDE;
  } else {
    if (gps.location.isValid()) {
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
    } else {
      currentLat = 0;
      currentLon = 0;
    }
  }

  printSensorData(envTemp, humidity, bodyTemp, accelX, heartRate, currentLat, currentLon);

  // GỬI PACKET VỚI ĐẦY ĐỦ THAM SỐ (bao gồm lat, lon)
  sendBinaryPacket(envTemp, humidity, bodyTemp, accelX, accelY, accelZ, 
                   gyroX, gyroY, gyroZ, heartRate, currentLat, currentLon);
  
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
void printSensorData(float envTemp, float humidity, float bodyTemp, 
                     int16_t accelX, int heartRate, double lat, double lon) {
  Serial.print(F("ENV  Temp: "));
  printFloat(envTemp, 1, F("C"));
  Serial.print(F("  Humid: "));
  printFloat(humidity, 1, F("%"));
  Serial.println();
  
  Serial.print(F("BODY Temp: "));
  if (!isnan(bodyTemp)) {
    Serial.print(bodyTemp, 2);
    Serial.print(F(" C"));
    
    if (bodyTemp > 37.5) {
      Serial.print(F(" [WARNING: FEVER]"));
    } else if (bodyTemp < 36.0) {
      Serial.print(F(" [WARNING: LOW]"));
    } else {
      Serial.print(F(" [NORMAL]"));
    }
  } else {
    Serial.print(F("N/A"));
  }
  Serial.println();
  
  Serial.print(F("HEART Rate: "));
  if (!hrFingerDetected) {
    Serial.print(F("N/A (No contact)"));
  } else {
    if (hrStable && hrStableAvg > 0) {
      Serial.print(hrStableAvg);
      Serial.print(F(" BPM [STABLE]"));
    } else if (hrAvg > 0) {
      Serial.print(hrAvg);
      Serial.print(F(" BPM [MEASURING]"));
    } else {
      Serial.print(F("Measuring..."));
    }
  }
  Serial.println();
  
  Serial.print(F("MOTION: Level="));
  Serial.print(activityLevel, 1);
  Serial.print(F(" | Steps: "));
  Serial.println((int)stepCount);
  
  Serial.print(F("GPS: "));
  if (USE_FIXED_GPS) {
    Serial.print(lat, 6);
    Serial.print(F(", "));
    Serial.print(lon, 6);
  } else {
    if (lat != 0 && lon != 0) {
      Serial.print(lat, 6);
      Serial.print(F(", "));
      Serial.print(lon, 6);
      Serial.print(F(" (Sat: "));
      Serial.print(gps.satellites.value());
      Serial.println(F(")"));
    } else {
      Serial.print(F("Waiting... (Sat: "));
      Serial.print(gps.satellites.value());
      Serial.println(F(")"));
    }
  }
}

void printFloat(float value, int decimals, const __FlashStringHelper* unit) {
  if (isnan(value)) {
    Serial.print(F("N/A"));
  } else {
    Serial.print(value, decimals);
    Serial.print(unit);
  }
}

// ==========================================
// GỬI BINARY PACKET (with gyro data for Gateway TinyML)
// ==========================================
void sendBinaryPacket(float envTemp, float humidity, float bodyTemp,
                      int16_t accelX, int16_t accelY, int16_t accelZ,
                      int16_t gyroX, int16_t gyroY, int16_t gyroZ,
                      int heartRate, double lat, double lon) {
  LoRaPacket pkt;

  pkt.magic   = MAGIC_BYTE;
  pkt.counter = packetCounter++;

  pkt.temp   = isnan(envTemp)  ? -9999 : (int16_t)(envTemp * 10);
  pkt.humid  = isnan(humidity) ? 0     : (uint16_t)(humidity * 10);
  pkt.tempDS = isnan(bodyTemp) ? -9999 : (int16_t)(bodyTemp * 10);

  // Sử dụng tọa độ được truyền vào
  if (lat != 0 && lon != 0) {
    pkt.lat = (int32_t)(lat * 1000000);
    pkt.lon = (int32_t)(lon * 1000000);
  } else {
    pkt.lat = 0;
    pkt.lon = 0;
  }

  pkt.ax = accelX;
  pkt.ay = accelY;
  pkt.az = accelZ;
  
  pkt.gx = gyroX;
  pkt.gy = gyroY;
  pkt.gz = gyroZ;

  pkt.hr = (heartRate > 0 && heartRate < 255) ? (uint8_t)heartRate : 0;
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
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(433E6)) {
    Serial.println(F("FAILED!"));
    while (1);
  }
  
  LoRa.setSpreadingFactor(10);
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.setTxPower(20);
  
  Serial.println(F("OK (SF10, BW250k, 20dBm)"));
}

// ==========================================
// KHỞI TẠO DHT22
// ==========================================
void initDHT() {
  dht.begin();
  Serial.println(F("[DHT22] OK"));
}

// ==========================================
// KHỞI TẠO DS18B20
// ==========================================
void initDS18B20() {
  Serial.print(F("[DS18B20] "));
  ds18b20.begin();
  
  int count = ds18b20.getDeviceCount();
  if (count > 0) {
    ds18b20.setResolution(12);
    ds18b20Available = true;
    Serial.print(F("OK ("));
    Serial.print(count);
    Serial.println(F(" device)"));
  } else {
    ds18b20Available = false;
    Serial.println(F("NOT FOUND"));
  }
}

// ==========================================
// KHỞI TẠO MPU6050
// ==========================================
void initMPU6050() {
  Serial.print(F("[MPU6050] "));
  
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() == 0) {
    Wire.beginTransmission(0x68);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(50);
    
    mpu.initialize();
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
    mpu.setDLPFMode(MPU6050_DLPF_BW_20);
    
    mpuAvailable = true;
    Serial.println(F("OK"));
  } else {
    mpuAvailable = false;
    Serial.println(F("NOT FOUND"));
  }
}

// ==========================================
// KHỞI TẠO MAX30102
// ==========================================
void initMAX30102() {
  Serial.print(F("[MAX30102] "));
  
  if (particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    particleSensor.setup(
      60,   // brightness
      4,    // sample average
      2,    // mode: Red+IR
      400,  // sample rate
      411,  // pulse width
      4096  // ADC range
    );
    
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeIR(0x0A);
    particleSensor.setPulseAmplitudeGreen(0x00);
    
    maxAvailable = true;
    Serial.println(F("OK"));
  } else {
    maxAvailable = false;
    Serial.println(F("NOT FOUND"));
  }
}

// ==========================================
// CALIBRATE GYRO
// ==========================================
void calibrateGyro() {
  Serial.println(F("[Gyro] Calibrating (keep still 2s)..."));
  
  const int samples = 300;
  long gxSum = 0, gySum = 0, gzSum = 0;
  
  delay(500);
  
  for (int i = 0; i < samples; i++) {
    int16_t gx, gy, gz;
    mpu.getRotation(&gx, &gy, &gz);
    gxSum += gx;
    gySum += gy;
    gzSum += gz;
    delay(6);
  }
  
  gxOffset = gxSum / samples;
  gyOffset = gySum / samples;
  gzOffset = gzSum / samples;
  gyroCalibrated = true;
  
  Serial.print(F("[Gyro] Offset: "));
  Serial.print(gxOffset);
  Serial.print(F(", "));
  Serial.print(gyOffset);
  Serial.print(F(", "));
  Serial.println(gzOffset);
}

// ==========================================
// SCAN I2C
// ==========================================
void scanI2C() {
  Serial.println(F("[I2C] Scanning..."));
  int count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  Found: 0x"));
      if (addr < 16) Serial.print('0');
      Serial.print(addr, HEX);
      
      if (addr == 0x68) Serial.print(F(" (MPU6050)"));
      else if (addr == 0x57) Serial.print(F(" (MAX30102)"));
      
      Serial.println();
      count++;
    }
  }
  
  if (count == 0) Serial.println(F("  No devices found"));
}

// ==========================================
// HEADER
// ==========================================
void printHeader() {
  Serial.println(F("\n====================================="));
  Serial.println(F("  HUMAN ACTIVITY MONITOR v1.1"));
  Serial.println(F("  CONTINUOUS SEND MODE"));
  Serial.println(F("  TinyML-Powered Motion Detection"));
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
  Serial.println(F("  Activities: Lying, Sitting, Standing,"));
  Serial.println(F("              Walking, Running"));
  Serial.println(F("=====================================\n"));
}