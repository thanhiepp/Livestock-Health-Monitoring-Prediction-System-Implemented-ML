#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST77xx.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// TinyML Model for Human Activity Classification
#include "human_activity_model.h"

// ==========================================
// CẤU HÌNH WIFI & SERVER
const char* ssid     = "marco";
const char* password = "hiep6703";  
const char* serverName = "http://172.20.10.8/lora_monitor/api/add_data.php";

// ==========================================
// CẤU HÌNH PHẦN CỨNG
// ==========================================
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define LORA_SS    15
#define LORA_RST   14
#define LORA_DIO0  27
#define LORA_FREQ  433E6
#define LORA_SF    10
#define LORA_BW    250E3
#define LORA_CR    5
#define LORA_SW    0x12

// ==========================================
// MAGIC BYTES
// ==========================================
#define MAGIC_BYTE_USER1 0xA1  // User 1
#define MAGIC_BYTE_USER2 0xA2  // User 2

// ==========================================
// BẢNG MÀU
// ==========================================
#define C_BG           0x0841
#define C_CARD         0x1082
#define C_PRIMARY      0x07FF
#define C_SUCCESS      0x07E0
#define C_WARNING      0xFD20
#define C_DANGER       0xF800
#define C_TEXT_PRIMARY 0xFFFF
#define C_TEXT_SECONDARY 0x8410
#define C_HEADER       0x4A69
#define C_DIVIDER      0x2945
#define DS_OFFSET_C  3.5f   
#define FIXED_LAT  15.97458646752691
#define FIXED_LON  108.25235621042664

// Activity colors - 3 classes
#define C_NGHI       0x528A  // Blue-gray (Rest)
#define C_DUNG       0x07E0  // Green (Standing)
#define C_DI_CHUYEN  0xFD20  // Orange (Moving)

// Activity colors for display (3 classes)
uint16_t ACTIVITY_COLORS[] = {
  0x528A,   // NGHI - Blue-gray (Rest)
  0x07E0,   // DUNG - Green (Standing)
  0xFD20    // DI_CHUYEN - Orange (Moving)
};

// ==========================================
// STRUCT BINARY PACKET - USER 1 & 2 (27 bytes - raw sensor data)
// ==========================================
struct __attribute__((packed)) LoRaPacketUser {
  uint8_t  magic;        // 0xA1 or 0xA2
  uint16_t counter;
  int16_t  temp;         // DHT22 temp x10
  uint16_t humid;        // DHT22 humidity x10
  int16_t  tempDS;       // DS18B20 temp x10
  int32_t  lat;
  int32_t  lon;

  // MPU6050 - Accelerometer
  int16_t  ax;
  int16_t  ay;
  int16_t  az;

  // MPU6050 - Gyroscope (for TinyML)
  int16_t  gx;
  int16_t  gy;
  int16_t  gz;  

  uint8_t  hr;           // Heart rate
  int8_t   rssi;         // RSSI từ node
};

// ==========================================
// STRUCT DỮ LIỆU HIỂN THỊ
// ==========================================
struct SensorData {
  uint8_t userId = 0;
  int packetCounter = 0;
  float temperature = NAN;
  float humidity = NAN;
  float bodyTemp = NAN;

  int16_t accelX = 0;
  int16_t accelY = 0;
  int16_t accelZ = 0;
  int16_t gyroX = 0;
  int16_t gyroY = 0;
  int16_t gyroZ = 0;

  int heartRate = 0;
  int rssi = -120;
  float snr = 0;
  unsigned long lastUpdate = 0;
  bool hasData = false;
  uint8_t activityClass = 2;  // Default: Standing
  uint8_t activityConf  = 0;

  int timeHour = 0;
  int timeMin = 0;
  int timeSec = 0;
};

SensorData user1Data;
SensorData user2Data;
uint8_t currentDisplayUser = 1;

volatile bool httpBusy = false;

// ==========================================
// KHAI BÁO HÀM
// ==========================================
void setupWifi();
void sendToServer(SensorData &data);
void receivePacket(int packetSize);
void drawModernInterfaceForNode();
void updateDisplayValuesForNode(SensorData &data);
void drawCompactCard(int x, int y, int w, int h, const char* label, float value, const char* unit, uint16_t color, bool isValid);
void drawIconWiFi(int x, int y, bool connected, uint16_t color);
void tryReconnectWiFi();
void drawSplashScreen();
void animateStartup();
void switchDisplayUser();
void printHex(const uint8_t *buf, size_t len);
void drawActivityCard(int x, int y, int w, int h, uint8_t activityClass, uint8_t confidence);
void drawNode2Layout(int startY, SensorData &data);

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== HUMAN ACTIVITY GATEWAY v1.0 ===");
  Serial.println("TinyML-Powered Human Motion Monitoring");
  Serial.print("  Packet size: ");
  Serial.print(sizeof(LoRaPacketUser));
  Serial.println(" bytes");

  // Khởi tạo TFT
  pinMode(TFT_CS, OUTPUT); 
  digitalWrite(TFT_CS, HIGH);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  
  drawSplashScreen();
  animateStartup();

  // Kết nối WiFi
  setupWifi();
  setupNTPTime();

  // Khởi tạo LoRa
  pinMode(LORA_SS, OUTPUT); 
  digitalWrite(LORA_SS, HIGH);
  pinMode(LORA_RST, OUTPUT); 
  digitalWrite(LORA_RST, LOW);
  delay(10);
  digitalWrite(LORA_RST, HIGH); 
  delay(50);

  SPI.begin(18, 19, 23, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[ERROR] LoRa init failed!");
    tft.fillScreen(C_DANGER);
    tft.setTextColor(C_TEXT_PRIMARY);
    tft.setCursor(20, 60);
    tft.setTextSize(1);
    tft.print("LoRa ERROR!");
    while (1);
  }

  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SW);
  LoRa.receive();
  
  Serial.println("[OK] LoRa ONLINE");
  Serial.print(" SF="); Serial.print(LORA_SF);
  Serial.print(" BW="); Serial.print((int)(LORA_BW/1000)); Serial.print("kHz");
  Serial.print(" CR=4/"); Serial.print(LORA_CR);
  Serial.print(" SW=0x"); Serial.println(LORA_SW, HEX);

  delay(500);
  drawModernInterfaceForNode();
}

// ==========================================
// LOOP
// ==========================================
unsigned long lastWiFiCheck = 0;
unsigned long lastUIUpdate = 0;
unsigned long lastUserSwitch = 0;

void loop() {
  if (millis() - lastWiFiCheck > 5000) {
    lastWiFiCheck = millis();
    tryReconnectWiFi();
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    receivePacket(packetSize);
    lastUIUpdate = millis();
  }

  if (millis() - lastUserSwitch > 6000) {
    lastUserSwitch = millis();
    switchDisplayUser();
  }

  if (millis() - lastUIUpdate > 1000) {
    lastUIUpdate = millis();
    drawIconWiFi(140, 4, WiFi.status() == WL_CONNECTED, C_PRIMARY);
  }
}

// ==========================================
// HÀM NHẬN PACKET
// ==========================================
void receivePacket(int packetSize) {
  Serial.println("\n==========================================");
  Serial.print("[RECV] Size: ");
  Serial.print(packetSize);
  Serial.println(" bytes");
  
  if (packetSize <= 0) {
    Serial.println("[WARN] packetSize <= 0");
    LoRa.receive();
    return;
  }

  uint8_t buf[64];
  if (packetSize > (int)sizeof(buf)) {
    Serial.println("[ERROR] packetSize > buffer");
    LoRa.receive();
    return;
  }

  int readCount = LoRa.readBytes(buf, packetSize);
  if (readCount != packetSize) {
    Serial.print("[ERROR] readBytes mismatch: ");
    Serial.print(readCount);
    Serial.print(" != ");
    Serial.println(packetSize);
    LoRa.receive();
    return;
  }

  uint8_t firstByte = buf[0];
  
  if (firstByte < 16) Serial.print("0");
  Serial.println(firstByte, HEX);

  // USER 1
if (firstByte == MAGIC_BYTE_USER1 && packetSize == (int)sizeof(LoRaPacketUser)) {
    LoRaPacketUser pkt;
    memcpy(&pkt, buf, sizeof(pkt));
    
    user1Data.userId = 1;
    user1Data.rssi = LoRa.packetRssi();
    user1Data.snr  = LoRa.packetSnr();
    user1Data.packetCounter = pkt.counter;

    user1Data.temperature = (pkt.temp   != -9999) ? (pkt.temp   / 10.0) : NAN;
    user1Data.humidity    = (pkt.humid  >      0) ? (pkt.humid  / 10.0) : NAN;
    user1Data.bodyTemp = (pkt.tempDS != -9999) ? ((pkt.tempDS / 10.0) + DS_OFFSET_C) : NAN;

    user1Data.accelX = pkt.ax;
    user1Data.accelY = pkt.ay;
    user1Data.accelZ = pkt.az;
    user1Data.gyroX = pkt.gx;
    user1Data.gyroY = pkt.gy;
    user1Data.gyroZ = pkt.gz;

    user1Data.heartRate = pkt.hr;
    
    // TinyML Inference on Gateway
    uint8_t confidence;
    user1Data.activityClass = predictActivity(
      pkt.ax, pkt.ay, pkt.az,
      pkt.gx, pkt.gy, pkt.gz,
      &confidence
    );
    user1Data.activityConf = confidence;
    
    user1Data.lastUpdate = millis();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      user1Data.timeHour = timeinfo.tm_hour;
      user1Data.timeMin = timeinfo.tm_min;
      user1Data.timeSec = timeinfo.tm_sec;
    }
    user1Data.hasData = true;
    
    Serial.println("\n=== USER 1 DATA ===");
    Serial.print("Packet #: "); Serial.println(user1Data.packetCounter);
    Serial.print("[TinyML] Activity: "); 
    Serial.print(getActivityLabelVN(user1Data.activityClass));
    Serial.print(" ("); Serial.print(user1Data.activityConf); Serial.println("%)");
    Serial.print("Body Temp: "); 
    if (!isnan(user1Data.bodyTemp)) Serial.print(user1Data.bodyTemp, 2);
    else Serial.print("N/A");
    Serial.println(" C");
    Serial.print("Heart Rate: "); Serial.print(user1Data.heartRate); Serial.println(" BPM");
    Serial.print("RSSI: "); Serial.print(user1Data.rssi); Serial.println(" dBm");
    
    // 🔧 FIX: Override GPS với tọa độ cố định
    Serial.print("GPS: FIXED -> Lat: "); 
    Serial.print(FIXED_LAT, 8);
    Serial.print(", Lon: ");
    Serial.println(FIXED_LON, 8);
    
    Serial.println("===================");
    sendToServer(user1Data);
    
    if (currentDisplayUser == 1) {
      updateDisplayValuesForNode(user1Data);
    }
}
  // USER 2
  else if (firstByte == MAGIC_BYTE_USER2 && packetSize == (int)sizeof(LoRaPacketUser)) {
    Serial.println("[IDENTIFY] ✓ USER 2");
    LoRaPacketUser pkt;
    memcpy(&pkt, buf, sizeof(pkt));
    
    user2Data.userId = 2;
    user2Data.rssi = LoRa.packetRssi();
    user2Data.snr = LoRa.packetSnr();
    user2Data.packetCounter = pkt.counter;
    user2Data.temperature = (pkt.temp != -9999) ? (pkt.temp / 10.0) : NAN;
    user2Data.humidity   = (pkt.humid > 0) ? (pkt.humid / 10.0) : NAN;
    user2Data.bodyTemp = (pkt.tempDS != -9999) ? ((pkt.tempDS / 10.0) + DS_OFFSET_C) : NAN;
    
    user2Data.accelX = pkt.ax;
    user2Data.accelY = pkt.ay;
    user2Data.accelZ = pkt.az;
    user2Data.gyroX = pkt.gx;
    user2Data.gyroY = pkt.gy;
    user2Data.gyroZ = pkt.gz;
    
    user2Data.heartRate = pkt.hr;
    
    // TinyML Inference on Gateway
    uint8_t confidence2;
    user2Data.activityClass = predictActivity(
      pkt.ax, pkt.ay, pkt.az,
      pkt.gx, pkt.gy, pkt.gz,
      &confidence2
    );
    user2Data.activityConf = confidence2;
    
    user2Data.lastUpdate = millis();
    struct tm timeinfo2;
    if (getLocalTime(&timeinfo2)) {
      user2Data.timeHour = timeinfo2.tm_hour;
      user2Data.timeMin = timeinfo2.tm_min;
      user2Data.timeSec = timeinfo2.tm_sec;
    }
    user2Data.hasData = true;

    Serial.println("\n=== USER 2 DATA ===");
    Serial.print("Packet #: ");  Serial.println(user2Data.packetCounter);
    Serial.print("[TinyML] Activity: "); 
    Serial.print(getActivityLabelVN(user2Data.activityClass));
    Serial.print(" ("); Serial.print(user2Data.activityConf); Serial.println("%)");
    Serial.print("Body Temp: ");
    if (!isnan(user2Data.bodyTemp)) Serial.print(user2Data.bodyTemp, 2);
    else Serial.print("N/A");
    Serial.println(" C");
    Serial.print("Env Temp: ");
    if (!isnan(user2Data.temperature)) Serial.print(user2Data.temperature, 1);
    else Serial.print("N/A");
    Serial.println(" C");
    Serial.print("Humidity: ");
    if (!isnan(user2Data.humidity)) Serial.print(user2Data.humidity, 0);
    else Serial.print("N/A");
    Serial.println(" %");
    Serial.print("RSSI: "); Serial.print(user2Data.rssi); Serial.println(" dBm");
    Serial.println("===================");

    sendToServer(user2Data);

    if (currentDisplayUser == 2) {
      updateDisplayValuesForNode(user2Data);
    }
  } 
  else {
    Serial.println("[WARN] ⚠️ Unknown packet or size mismatch");
    Serial.print("  Size: "); Serial.print(packetSize);
    Serial.print(", Magic: 0x"); 
    if (firstByte < 16) Serial.print("0");
    Serial.println(firstByte, HEX);
  }
  
  Serial.println("==========================================\n");
  LoRa.receive();
}

// ==========================================
// CHUYỂN ĐỔI USER
// ==========================================
void switchDisplayUser() {
  currentDisplayUser = (currentDisplayUser == 1) ? 2 : 1;
  Serial.print("[UI] Switch to User ");
  Serial.println(currentDisplayUser);
  
  drawModernInterfaceForNode();
  
  SensorData &data = (currentDisplayUser == 1) ? user1Data : user2Data;
  if (data.hasData) {
    updateDisplayValuesForNode(data);
  }
}

// ==========================================
// HÀM WIFI
// ==========================================
void setupWifi() {
  tft.fillRect(0, 100, 160, 20, C_BG);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setTextSize(1);
  tft.setCursor(10, 105);
  tft.print("Connecting WiFi...");
  
  delay(10);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    tft.fillCircle(80 + (retry % 3) * 10, 115, 2, C_PRIMARY);
    retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi Connected");
    Serial.print("IP: "); 
    Serial.println(WiFi.localIP());
    
    tft.fillRect(0, 100, 160, 20, C_BG);
    tft.setTextColor(C_SUCCESS);
    tft.setCursor(20, 105);
    tft.print("WiFi Connected!");
    delay(1000);
  } else {
    Serial.println("\n[ERROR] WiFi Failed");
    tft.fillRect(0, 100, 160, 20, C_BG);
    tft.setTextColor(C_DANGER);
    tft.setCursor(20, 105);
    tft.print("WiFi Failed!");
    delay(1000);
  }
}

void setupNTPTime() {
  Serial.print("[NTP] Syncing time...");
  
  // Cấu hình NTP cho múi giờ Việt Nam (UTC+7)
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  
  if (retry < 20) {
    Serial.println(" OK");
    Serial.print("[TIME] ");
    Serial.print(timeinfo.tm_hour);
    Serial.print(":");
    Serial.print(timeinfo.tm_min);
    Serial.print(":");
    Serial.println(timeinfo.tm_sec);
  } else {
    Serial.println(" FAILED (will use anyway)");
  }
}

void tryReconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    
    int t = 0;
    while (WiFi.status() != WL_CONNECTED && t < 8) {
      delay(500);
      Serial.print(".");
      t++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WiFi] Reconnected");
    }
  }
}

// ==========================================
// GỬI DỮ LIỆU LÊN SERVER
// ==========================================
void sendToServer(SensorData &data) {
  if (WiFi.status() != WL_CONNECTED || httpBusy) {
    return;
  }

  String postData = "node_id=" + String(data.userId)
                    + "&packet_counter=" + String(data.packetCounter)
                    + "&temperature=" + (isnan(data.temperature) ? String("") : String(data.temperature, 1))
                    + "&humidity=" + (isnan(data.humidity) ? String("") : String(data.humidity, 1))
                    + "&bodyTemp=" + (isnan(data.bodyTemp) ? String("") : String(data.bodyTemp, 2))
                    + "&heartRate=" + String(data.heartRate)
                    + "&accelX=" + String(data.accelX)
                    + "&accelY=" + String(data.accelY)
                    + "&accelZ=" + String(data.accelZ)
                    + "&gyroX=" + String(data.gyroX)
                    + "&gyroY=" + String(data.gyroY)
                    + "&gyroZ=" + String(data.gyroZ)
                    + "&rssi=" + String(data.rssi)
                    + "&activityClass=" + String(data.activityClass)
                    + "&activityConf=" + String(data.activityConf)
                    + "&latitude=" + String(FIXED_LAT, 8)
                    + "&longitude=" + String(FIXED_LON, 8);

  HTTPClient http;
  httpBusy = true;
  
 if (http.begin(serverName)) {
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    
    if (httpCode > 0) {
      Serial.print("[SERVER] User ");
      Serial.print(data.userId);
      Serial.print(" - HTTP ");
      Serial.println(httpCode);
    } else {
      Serial.print("[SERVER] User ");
      Serial.print(data.userId);
      Serial.println(" - FAILED");
    }
    http.end();
  }
  
  httpBusy = false;
}

// ==========================================
// VẼ GIAO DIỆN
// ==========================================
void drawSplashScreen() {
  tft.fillScreen(C_BG);
  tft.setTextSize(2);
  tft.setTextColor(C_PRIMARY);
  tft.setCursor(5, 30);
  tft.print("HEALTH");
  tft.setCursor(10, 50);
  tft.print("MONITOR");
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(25, 75);
  tft.print("TinyML v1.0");
}

void animateStartup() {
  for (int i = 0; i < 3; i++) {
    tft.fillCircle(60 + i * 15, 100, 3, C_PRIMARY);
    delay(200);
  }
  delay(500);
}

void drawModernInterfaceForNode() {
  tft.fillScreen(C_BG);
  
  // Header với indicator sensor
  tft.fillRect(0, 0, 160, 16, C_HEADER);
  tft.drawFastHLine(0, 16, 160, C_PRIMARY);
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_PRIMARY);
  tft.setCursor(4, 4);
  tft.print("USER ");
  tft.print(currentDisplayUser);
  
  // Icon cảm biến
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(58, 4);
  if (currentDisplayUser == 1) {
    tft.print("[ALL]");
  } else {
    tft.print("[HEALTH]");
  }
  
  drawIconWiFi(140, 4, false, C_TEXT_SECONDARY);
  
  // Status bar
  tft.fillRect(2, 20, 156, 16, C_CARD);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(8, 24);
  tft.print("Waiting User ");
  tft.print(currentDisplayUser);
  tft.print("...");
  
  if (currentDisplayUser == 1) {
    // Node 1: Layout cũ (full sensors)
    drawActivityCard(2, 40, 156, 35, 2, 0);
    drawCompactCard(2, 79, 76, 24, "BODY", 0, "C", C_WARNING, false);
    drawCompactCard(82, 79, 76, 24, "HR", 0, "BPM", C_DANGER, false);
    drawCompactCard(2, 107, 76, 21, "TEMP", 0, "C", C_SUCCESS, false);
    drawCompactCard(82, 107, 76, 21, "HUM", 0, "%", C_PRIMARY, false);
  } else {
    // Node 2: Layout mới - 3 cards nhiệt độ
    // Placeholder - sẽ được update bởi drawNode2Layout()
  }
}

void updateDisplayValuesForNode(SensorData &data) {
  // Status bar
  tft.fillRect(2, 20, 156, 16, C_SUCCESS);
  tft.setTextColor(C_TEXT_PRIMARY);
  tft.setTextSize(1);
  tft.setCursor(6, 24);

  if (data.timeHour < 10) tft.print('0');
  tft.print(data.timeHour);
  tft.print(':');
  if (data.timeMin < 10) tft.print('0');
  tft.print(data.timeMin);
  tft.print(':');
  if (data.timeSec < 10) tft.print('0');
  tft.print(data.timeSec);

  tft.print(" P:");
  tft.print(data.packetCounter);
  tft.print(" ");
  tft.print(data.rssi);
  tft.print("dBm");
  
  if (data.userId == 1) {
    // Node 1: Hiển thị đầy đủ
    drawActivityCard(2, 40, 156, 35, data.activityClass, data.activityConf);
    
    bool bodyTempValid = !isnan(data.bodyTemp);
    drawCompactCard(2, 79, 76, 24, "BODY", data.bodyTemp, "C", C_WARNING, bodyTempValid);
    
    bool hrValid = (data.heartRate > 0 && data.heartRate < 255);
    drawCompactCard(82, 79, 76, 24, "HR", data.heartRate, "BPM", C_DANGER, hrValid);
    
    bool envTempValid = !isnan(data.temperature);
    drawCompactCard(2, 107, 76, 21, "TEMP", data.temperature, "C", C_SUCCESS, envTempValid);
    
    bool humValid = !isnan(data.humidity);
    drawCompactCard(82, 107, 76, 21, "HUM", data.humidity, "%", C_PRIMARY, humValid);
    
  } else {
    // Node 2: Layout mới - 3 cards nhiệt độ & độ ẩm
    drawNode2Layout(40, data);
  }
}

// ==========================================
// NODE 2 - LAYOUT MỚI (3 CARDS)
// ==========================================
void drawNode2Layout(int startY, SensorData &data) {
  int cardHeight = 28;
  int spacing = 2;
  
  // === CARD 1: NHIỆT ĐỘ MÔI TRƯỜNG ===
  int card1Y = startY;
  bool envTempValid = !isnan(data.temperature);
  
  tft.fillRoundRect(2, card1Y, 156, cardHeight, 4, C_CARD);
  tft.drawRoundRect(2, card1Y, 156, cardHeight, 4, C_SUCCESS);
  
  // Icon nhiệt kế
  tft.fillCircle(12, card1Y + 18, 3, C_SUCCESS);
  tft.fillRect(10, card1Y + 8, 4, 10, C_SUCCESS);
  tft.drawRect(9, card1Y + 6, 6, 14, C_SUCCESS);
  
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(22, card1Y + 4);
  tft.print("ENV TEMP");
  
  if (envTempValid) {
    tft.setTextSize(2);
    tft.setTextColor(C_SUCCESS);
    tft.setCursor(22, card1Y + 14);
    tft.print(data.temperature, 1);
    tft.setTextSize(1);
    tft.print(" C");
  } else {
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_SECONDARY);
    tft.setCursor(70, card1Y + 14);
    tft.print("N/A");
  }
  
  // === CARD 2: NHIỆT ĐỘ CƠ THỂ ===
  int card2Y = card1Y + cardHeight + spacing;
  bool bodyTempValid = !isnan(data.bodyTemp);
  
  uint16_t bodyColor = C_WARNING;
  if (bodyTempValid) {
    if (data.bodyTemp > 37.5) bodyColor = C_DANGER;
    else if (data.bodyTemp >= 36.0) bodyColor = C_SUCCESS;
    else bodyColor = 0x049F; // Blue cho thấp
  }
  
  tft.fillRoundRect(2, card2Y, 156, cardHeight, 4, C_CARD);
  tft.drawRoundRect(2, card2Y, 156, cardHeight, 4, bodyColor);
  
  // Icon tim/body
  tft.fillCircle(12, card2Y + 14, 4, bodyColor);
  tft.fillCircle(8, card2Y + 11, 2, bodyColor);
  tft.fillCircle(16, card2Y + 11, 2, bodyColor);
  
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(22, card2Y + 4);
  tft.print("BODY TEMP");
  
  if (bodyTempValid) {
    tft.setTextSize(2);
    tft.setTextColor(bodyColor);
    tft.setCursor(22, card2Y + 14);
    tft.print(data.bodyTemp, 1);
    tft.setTextSize(1);
    tft.print(" C");
    
    // Status indicator
    tft.setTextColor(C_TEXT_SECONDARY);
    tft.setCursor(115, card2Y + 18);
    if (data.bodyTemp > 37.5) {
      tft.setTextColor(C_DANGER);
      tft.print("HIGH");
    } else if (data.bodyTemp < 36.0) {
      tft.setTextColor(0x049F);
      tft.print("LOW");
    } else {
      tft.setTextColor(C_SUCCESS);
      tft.print("NORM");
    }
  } else {
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_SECONDARY);
    tft.setCursor(70, card2Y + 14);
    tft.print("N/A");
  }
  
  // === CARD 3: ĐỘ ẨM ===
  int card3Y = card2Y + cardHeight + spacing;
  bool humValid = !isnan(data.humidity);
  
  tft.fillRoundRect(2, card3Y, 156, cardHeight, 4, C_CARD);
  tft.drawRoundRect(2, card3Y, 156, cardHeight, 4, C_PRIMARY);
  
  // Icon giọt nước
  tft.fillCircle(12, card3Y + 16, 3, C_PRIMARY);
  tft.fillCircle(12, card3Y + 12, 2, C_PRIMARY);
  tft.drawLine(12, card3Y + 8, 12, card3Y + 11, C_PRIMARY);
  
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(22, card3Y + 4);
  tft.print("HUMIDITY");
  
  if (humValid) {
    tft.setTextSize(2);
    tft.setTextColor(C_PRIMARY);
    tft.setCursor(22, card3Y + 14);
    tft.print(data.humidity, 0);
    tft.setTextSize(1);
    tft.print(" %");
  } else {
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_SECONDARY);
    tft.setCursor(70, card3Y + 14);
    tft.print("N/A");
  }
}

// ==========================================
// VẼ COMPACT CARD
// ==========================================
void drawCompactCard(int x, int y, int w, int h, const char* label, float value, const char* unit, uint16_t color, bool isValid) {
  tft.fillRoundRect(x, y, w, h, 3, C_CARD);
  tft.drawRoundRect(x, y, w, h, 3, color);
  
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(x + 4, y + 3);
  tft.print(label);
  
  if (isValid) {
    tft.setTextColor(color);
    tft.setCursor(x + 4, y + 13);
    tft.print(value, 1);
    tft.print(" ");
    tft.print(unit);
  } else {
    tft.setTextColor(C_TEXT_SECONDARY);
    tft.setCursor(x + w/2 - 10, y + 13);
    tft.print("N/A");
  }
}

// ==========================================
// VẼ ACTIVITY CARD
// ==========================================
void drawActivityCard(int x, int y, int w, int h, uint8_t activityClass, uint8_t confidence) {
  uint16_t actColor = ACTIVITY_COLORS[activityClass];
  
  tft.fillRoundRect(x, y, w, h, 4, C_CARD);
  tft.drawRoundRect(x, y, w, h, 4, actColor);
  
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_SECONDARY);
  tft.setCursor(x + 4, y + 4);
  tft.print("ACTIVITY");
  
  tft.setTextSize(2);
  tft.setTextColor(actColor);
  tft.setCursor(x + 4, y + 14);
  const char* label = getActivityLabelVN(activityClass);
  tft.print(label);
  
  if (confidence > 0) {
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT_SECONDARY);
    tft.setCursor(x + w - 30, y + h - 10);
    tft.print(confidence);
    tft.print("%");
  }
}

// ==========================================
// VẼ ICON WIFI
// ==========================================
void drawIconWiFi(int x, int y, bool connected, uint16_t color) {
  uint16_t c = connected ? C_SUCCESS : C_TEXT_SECONDARY;
  
  tft.fillCircle(x + 4, y + 8, 1, c);
  tft.drawCircle(x + 4, y + 8, 3, c);
  tft.drawCircle(x + 4, y + 8, 5, c);
  tft.drawCircle(x + 4, y + 8, 7, c);
}