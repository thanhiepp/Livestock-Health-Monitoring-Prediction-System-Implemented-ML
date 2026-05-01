/**
 * DATA COLLECTOR - 3 CLASSES
 * Thu thập dữ liệu cho model phân loại 3 hoạt động
 * 
 * MAPPING:
 * Class 0 (NGHI)      = LYING + SITTING
 * Class 1 (DUNG)      = STANDING  
 * Class 2 (DI_CHUYEN) = WALKING + RUNNING
 * 
 * HƯỚNG DẪN:
 * 1. Upload code lên Arduino
 * 2. Mở Serial Monitor (115200 baud)
 * 3. Nhập số để bắt đầu thu thập:
 *    0 = NGHI (nghỉ ngơi)
 *    1 = DUNG (đứng)
 *    2 = DI_CHUYEN (di chuyển)
 * 4. Thu mỗi class ít nhất 1200 samples
 */

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu(0x68);

// Gyro calibration offsets
int16_t gxOffset = 0, gyOffset = 0, gzOffset = 0;

// 3 Activity classes
const char* ACTIVITIES[] = {"NGHI", "DUNG", "DI_CHUYEN"};
const char* ACTIVITIES_DESC[] = {
  "NGHI      (Lying + Sitting)",
  "DUNG      (Standing)",
  "DI_CHUYEN (Walking + Running)"
};

// Target samples
const int TARGET_SAMPLES = 1200;  // 1200 samples x 3 classes = 3600 total
int samplesCollected[3] = {0, 0, 0};

// Recording state
int currentActivity = -1;
bool isRecording = false;
unsigned long recordStartTime = 0;
const unsigned long RECORD_DURATION = 60000; // 60 giây
int sessionSampleCount = 0;

// Sampling
const unsigned long SAMPLE_INTERVAL = 50; // 20Hz
unsigned long lastSample = 0;

// Quality tracking
float lastAccel[3] = {0, 0, 0};
int goodSampleCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Wire.begin();
  
  printHeader();
  initMPU6050();
  calibrateGyro();
  printProgress();
}

void loop() {
  // Check Serial input
  if (Serial.available() > 0) {
    char input = Serial.read();
    
    if (input >= '0' && input <= '2') {
      currentActivity = input - '0';
      startRecording();
    } else if (input == 's' || input == 'S') {
      stopRecording();
    } else if (input == 'p' || input == 'P') {
      printProgress();
    }
  }
  
  // Record samples
  if (isRecording && (millis() - lastSample >= SAMPLE_INTERVAL)) {
    lastSample = millis();
    recordSample();
    
    // Auto stop after duration
    if (millis() - recordStartTime >= RECORD_DURATION) {
      stopRecording();
    }
  }
}

void initMPU6050() {
  Serial.print(F("[MPU6050] Initializing... "));
  
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
    
    Serial.println(F("OK ✓"));
  } else {
    Serial.println(F("FAILED!"));
    while (1) {
      delay(1000);
      Serial.println(F("[ERROR] MPU6050 not found!"));
    }
  }
}

void calibrateGyro() {
  Serial.println(F("[Gyro] Calibrating (keep device still)..."));
  
  const int samples = 500;
  long gxSum = 0, gySum = 0, gzSum = 0;
  delay(500);
  
  for (int i = 0; i < samples; i++) {
    int16_t gx, gy, gz;
    mpu.getRotation(&gx, &gy, &gz);
    gxSum += gx;
    gySum += gy;
    gzSum += gz;
    delay(5);
    
    if (i % 100 == 0) Serial.print(".");
  }
  Serial.println();
  
  gxOffset = gxSum / samples;
  gyOffset = gySum / samples;
  gzOffset = gzSum / samples;
  
  Serial.print(F("[Gyro] Calibration complete. Offsets: "));
  Serial.print(gxOffset); Serial.print(F(", "));
  Serial.print(gyOffset); Serial.print(F(", "));
  Serial.println(gzOffset);
}

void printProgress() {
  Serial.println(F("\n╔══════════════════════════════════════════════════╗"));
  Serial.println(F("║       TIẾN ĐỘ THU THẬP DỮ LIỆU 3 CLASSES       ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  
  int totalCollected = 0;
  int totalTarget = TARGET_SAMPLES * 3;
  
  for (int i = 0; i < 3; i++) {
    totalCollected += samplesCollected[i];
    
    Serial.print(F("  "));
    Serial.print(ACTIVITIES_DESC[i]);
    Serial.print(F(": "));
    Serial.print(samplesCollected[i]);
    Serial.print(F(" / "));
    Serial.print(TARGET_SAMPLES);
    
    float percent = (float)samplesCollected[i] / TARGET_SAMPLES * 100;
    Serial.print(F(" ("));
    Serial.print(percent, 0);
    Serial.print(F("%)"));
    
    if (samplesCollected[i] >= TARGET_SAMPLES) {
      Serial.println(F(" ✓ COMPLETE"));
    } else {
      int remaining = TARGET_SAMPLES - samplesCollected[i];
      Serial.print(F(" - Need: "));
      Serial.print(remaining);
      Serial.println(F(" more"));
    }
  }
  
  Serial.println(F("\n────────────────────────────────────────────────────"));
  Serial.print(F("  TOTAL: "));
  Serial.print(totalCollected);
  Serial.print(F(" / "));
  Serial.print(totalTarget);
  Serial.print(F(" ("));
  Serial.print((float)totalCollected/totalTarget*100, 1);
  Serial.println(F("%)"));
  
  if (totalCollected >= totalTarget) {
    Serial.println(F("\n  🎉 HOÀN THÀNH! ĐỦ DỮ LIỆU ĐỂ TRAINING!"));
  }
  
  Serial.println(F("══════════════════════════════════════════════════\n"));
  
  printInstructions();
}

void startRecording() {
  if (currentActivity < 0 || currentActivity > 2) {
    Serial.println(F("[ERROR] Invalid activity"));
    return;
  }
  
  isRecording = true;
  recordStartTime = millis();
  lastSample = millis();
  sessionSampleCount = 0;
  goodSampleCount = 0;
  
  Serial.println(F("\n╔══════════════════════════════════════════════════╗"));
  Serial.print(F("║  RECORDING: "));
  Serial.print(ACTIVITIES[currentActivity]);
  Serial.println(F("                                      ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  
  // Detailed instructions per activity
  Serial.println(F("\n📋 CHI TIẾT THU THẬP:"));
  Serial.println(F("────────────────────────────────────────────────────"));
  
  switch(currentActivity) {
    case 0: // NGHI
      Serial.println(F("  CLASS 0: NGHI (Rest/Relax)"));
      Serial.println(F("  "));
      Serial.println(F("  🛏️  LYING (30 giây):"));
      Serial.println(F("     - 0-10s:  Nằm ngửa hoàn toàn yên"));
      Serial.println(F("     - 10-20s: Nằm nghiêng trái"));
      Serial.println(F("     - 20-30s: Nằm nghiêng phải"));
      Serial.println(F("  "));
      Serial.println(F("  💺 SITTING (30 giây):"));
      Serial.println(F("     - 30-40s: Ngồi thẳng lưng trên ghế"));
      Serial.println(F("     - 40-50s: Ngồi dựa lưng thoải mái"));
      Serial.println(F("     - 50-60s: Ngồi bệt xuống sàn"));
      Serial.println(F("  "));
      Serial.println(F("  ⚠️  LƯU Ý: GIỮ YÊN, KHÔNG DI CHUYỂN!"));
      break;
      
    case 1: // DUNG
      Serial.println(F("  CLASS 1: DUNG (Standing)"));
      Serial.println(F("  "));
      Serial.println(F("  🧍 STANDING (60 giây):"));
      Serial.println(F("     - 0-30s:  Đứng yên hoàn toàn"));
      Serial.println(F("     - 30-45s: Đứng, chuyển trọng tâm nhẹ"));
      Serial.println(F("     - 45-60s: Đứng, xoay người nhẹ trái/phải"));
      Serial.println(F("  "));
      Serial.println(F("  ⚠️  LƯU Ý: KHÔNG DI CHUYỂN CHÂN, CHỈ ĐỨNG!"));
      break;
      
    case 2: // DI_CHUYEN
      Serial.println(F("  CLASS 2: DI_CHUYEN (Moving)"));
      Serial.println(F("  "));
      Serial.println(F("  🚶 WALKING (30 giây):"));
      Serial.println(F("     - 0-15s:  Đi bộ chậm, tốc độ đều"));
      Serial.println(F("     - 15-30s: Đi bộ vừa, tốc độ đều"));
      Serial.println(F("  "));
      Serial.println(F("  🏃 RUNNING (30 giây):"));
      Serial.println(F("     - 30-45s: Chạy bộ vừa phải"));
      Serial.println(F("     - 45-60s: Chạy nhanh hoặc chạy tại chỗ"));
      Serial.println(F("  "));
      Serial.println(F("  ⚠️  LƯU Ý: LUÔN DI CHUYỂN, KHÔNG DỪNG LẠI!"));
      break;
  }
  
  Serial.println(F("════════════════════════════════════════════════════"));
  Serial.println(F("\nCSV Format: accelX,accelY,accelZ,gyroX,gyroY,gyroZ,activity"));
  Serial.println();
}

void stopRecording() {
  if (!isRecording) return;
  
  isRecording = false;
  samplesCollected[currentActivity] += sessionSampleCount;
  
  Serial.println(F("\n╔══════════════════════════════════════════════════╗"));
  Serial.print(F("║  STOPPED - Collected "));
  Serial.print(sessionSampleCount);
  Serial.println(F(" samples                    ║"));
  
  // Quality assessment
  float qualityPercent = (float)goodSampleCount / sessionSampleCount * 100;
  Serial.print(F("║  Quality: "));
  Serial.print(qualityPercent, 0);
  Serial.print(F("%"));
  
  if (qualityPercent > 95) {
    Serial.println(F(" - EXCELLENT! ✓✓✓              ║"));
  } else if (qualityPercent > 85) {
    Serial.println(F(" - GOOD ✓✓                     ║"));
  } else if (qualityPercent > 75) {
    Serial.println(F(" - OK ✓                        ║"));
  } else {
    Serial.println(F(" - POOR, COLLECT AGAIN ✗       ║"));
  }
  
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  
  printProgress();
  currentActivity = -1;
}

void recordSample() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Apply gyro offset
  gx -= gxOffset;
  gy -= gyOffset;
  gz -= gzOffset;
  
  // Quality check - detect noise
  float accel[3] = {(float)ax, (float)ay, (float)az};
  bool isNoisy = false;
  
  if (sessionSampleCount > 0) {
    // Check for sudden large changes (noise)
    for (int i = 0; i < 3; i++) {
      float change = abs(accel[i] - lastAccel[i]);
      if (change > 12000) {  // Threshold for noise
        isNoisy = true;
        break;
      }
    }
  }
  
  if (!isNoisy) {
    goodSampleCount++;
  }
  
  // Save for next comparison
  for (int i = 0; i < 3; i++) {
    lastAccel[i] = accel[i];
  }
  
  // Output CSV format
  Serial.print(ax); Serial.print(",");
  Serial.print(ay); Serial.print(",");
  Serial.print(az); Serial.print(",");
  Serial.print(gx); Serial.print(",");
  Serial.print(gy); Serial.print(",");
  Serial.print(gz); Serial.print(",");
  Serial.println(currentActivity);
  
  sessionSampleCount++;
  
  // Progress indicator every 200 samples
  if (sessionSampleCount % 200 == 0) {
    unsigned long elapsed = millis() - recordStartTime;
    Serial.print(F("# Progress: "));
    Serial.print(sessionSampleCount);
    Serial.print(F(" samples ("));
    Serial.print(elapsed / 1000);
    Serial.print(F("s) - Quality: "));
    Serial.print((float)goodSampleCount/sessionSampleCount*100, 0);
    Serial.println(F("%"));
  }
}

void printHeader() {
  Serial.println(F("\n╔══════════════════════════════════════════════════╗"));
  Serial.println(F("║    DATA COLLECTOR - 3 CLASSES VERSION          ║"));
  Serial.println(F("║    NGHI | DUNG | DI_CHUYEN                     ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  Serial.println(F("\n  Target: 1200 samples per class = 3600 total\n"));
}

void printInstructions() {
  Serial.println(F("┌──────────────────────────────────────────────────┐"));
  Serial.println(F("│  CHỌN HOẠT ĐỘNG ĐỂ THU THẬP:                   │"));
  Serial.println(F("├──────────────────────────────────────────────────┤"));
  Serial.println(F("│  0 = NGHI      (Lying + Sitting)                │"));
  Serial.println(F("│  1 = DUNG      (Standing)                        │"));
  Serial.println(F("│  2 = DI_CHUYEN (Walking + Running)               │"));
  Serial.println(F("│                                                  │"));
  Serial.println(F("│  P = Xem tiến độ (Progress)                     │"));
  Serial.println(F("│  S = Dừng ghi  (Stop)                           │"));
  Serial.println(F("└──────────────────────────────────────────────────┘"));
  Serial.println(F("\n💡 MẸO: Thu mỗi class 3-4 lần, mỗi lần 60 giây\n"));
}

/**
 * ═══════════════════════════════════════════════════════════
 * HƯỚNG DẪN THU THẬP TỐI ƯU CHO 3 CLASSES
 * ═══════════════════════════════════════════════════════════
 * 
 * 📊 MỤC TIÊU:
 * - NGHI:      1200 samples (3-4 sessions x 60s)
 * - DUNG:      1200 samples (3-4 sessions x 60s)
 * - DI_CHUYEN: 1200 samples (3-4 sessions x 60s)
 * - TOTAL:     3600 samples
 * 
 * 🎯 CHIẾN LƯỢC THU THẬP:
 * 
 * 1. CLASS 0 - NGHI (Rest):
 *    Session 1: Nằm ngửa → nghiêng trái → nghiêng phải
 *              + Ngồi ghế thẳng lưng
 *    Session 2: Nằm úp → nằm thu gối
 *              + Ngồi dựa lưng thoải mái
 *    Session 3: Nằm thư giãn
 *              + Ngồi bệt sàn
 *    → MỤC TIÊU: Capture tất cả tư thế TĨNH, KHÔNG DI CHUYỂN
 * 
 * 2. CLASS 1 - DUNG (Standing):
 *    Session 1: Đứng yên hoàn toàn
 *    Session 2: Đứng, thỉnh thoảng chuyển trọng tâm
 *    Session 3: Đứng, xoay người nhẹ trái/phải
 *    → MỤC TIÊU: Capture tư thế ĐỨNG, không bước chân
 * 
 * 3. CLASS 2 - DI_CHUYEN (Moving):
 *    Session 1: Đi bộ chậm → đi bộ vừa
 *    Session 2: Đi bộ nhanh → chạy nhẹ
 *    Session 3: Chạy vừa → chạy nhanh (hoặc chạy tại chỗ)
 *    → MỤC TIÊU: Capture mọi dạng DI CHUYỂN có bước chân
 * 
 * ⚠️  CỰC KỲ QUAN TRỌNG:
 * - Vị trí cảm biến: GẮN CỐ ĐỊNH Ở MỘT CHỖ (ví dụ: cổ tay trái)
 * - Hướng cảm biến: LUÔN GIỐNG NHAU mọi lần thu
 * - Môi trường: Bình thường, tránh rung lắc ngoại vi
 * - Người thu: Thu từ 2-3 người khác nhau nếu có thể
 * 
 * 📝 SAU KHI THU XONG:
 * 1. Copy tất cả dữ liệu từ Serial Monitor
 * 2. Lưu vào file: activity_data_3class.csv
 * 3. Xóa các dòng comment (bắt đầu bằng #)
 * 4. Thêm header: accelX,accelY,accelZ,gyroX,gyroY,gyroZ,activity
 * 5. Chạy: python train_3class_model.py activity_data_3class.csv
 */