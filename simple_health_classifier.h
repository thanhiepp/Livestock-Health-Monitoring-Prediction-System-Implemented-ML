#ifndef SIMPLE_HEALTH_CLASSIFIER_H
#define SIMPLE_HEALTH_CLASSIFIER_H

/*
 * Simple Rule-based Health Classifier
 * No TensorFlow Lite required!
 * 
 * Based on temperature and heart rate thresholds
 */

#include <Arduino.h>

// Health classes
#define HEALTH_HEALTHY      0
#define HEALTH_HEAT_STRESS  1
#define HEALTH_FEVER        2
#define HEALTH_ABNORMAL     3

// Temperature thresholds (Celsius)
#define TEMP_NORMAL_MIN     38.0f
#define TEMP_NORMAL_MAX     39.5f
#define TEMP_HEAT_MIN       39.6f
#define TEMP_HEAT_MAX       41.0f
#define TEMP_FEVER_MIN      40.5f
#define TEMP_LOW_MAX        37.9f

// Heart rate thresholds (BPM)
#define HR_NORMAL_MIN       60
#define HR_NORMAL_MAX       80
#define HR_ELEVATED_MIN     85
#define HR_HIGH_MIN         90
#define HR_VERY_HIGH        130

// Class labels
const char* HEALTH_LABELS_VN[] = {
  "KHOE MANH",      // 0
  "STRESS NHIET",   // 1
  "SOT",            // 2
  "BAT THUONG"      // 3
};

const char* HEALTH_LABELS_EN[] = {
  "Healthy",        // 0
  "Heat Stress",    // 1
  "Fever",          // 2
  "Abnormal"        // 3
};

// Colors (RGB565)
const uint16_t HEALTH_COLORS[] = {
  0x07E0,  // Green
  0xFD20,  // Orange
  0xF800,  // Red
  0x528A   // Blue-gray
};

/**
 * Simple rule-based health classification
 * 
 * Rules:
 * 1. FEVER: temp >= 40.5°C AND hr >= 90 BPM
 * 2. HEAT_STRESS: temp 39.6-41°C AND hr 85-120 BPM (not fever)
 * 3. HEALTHY: temp 38-39.5°C AND hr 60-80 BPM
 * 4. ABNORMAL: everything else
 */
uint8_t classifyHealthStatus(float bodyTemp, float heartRate, uint8_t* confidence) {
  // Default
  *confidence = 0;
  
  // Validate inputs
  if (isnan(bodyTemp) || isnan(heartRate) || bodyTemp <= 0 || heartRate <= 0) {
    *confidence = 0;
    return HEALTH_ABNORMAL;
  }
  
  // Rule 1: FEVER - High temp + high HR
  if (bodyTemp >= TEMP_FEVER_MIN && heartRate >= HR_HIGH_MIN) {
    // Confidence increases with severity
    float temp_severity = min(100.0f, (bodyTemp - TEMP_FEVER_MIN) * 50.0f);
    float hr_severity = min(100.0f, (heartRate - HR_HIGH_MIN) * 2.0f);
    *confidence = (uint8_t)((temp_severity + hr_severity) / 2.0f);
    *confidence = constrain(*confidence, 70, 100);
    return HEALTH_FEVER;
  }
  
  // Rule 2: HEAT STRESS - Elevated temp + elevated HR
  if (bodyTemp >= TEMP_HEAT_MIN && bodyTemp < TEMP_FEVER_MIN && 
      heartRate >= HR_ELEVATED_MIN && heartRate < HR_VERY_HIGH) {
    float temp_factor = (bodyTemp - TEMP_HEAT_MIN) / (TEMP_FEVER_MIN - TEMP_HEAT_MIN);
    float hr_factor = (heartRate - HR_ELEVATED_MIN) / (HR_VERY_HIGH - HR_ELEVATED_MIN);
    *confidence = (uint8_t)((temp_factor + hr_factor) * 50.0f);
    *confidence = constrain(*confidence, 60, 95);
    return HEALTH_HEAT_STRESS;
  }
  
  // Rule 3: HEALTHY - Normal ranges
  if (bodyTemp >= TEMP_NORMAL_MIN && bodyTemp <= TEMP_NORMAL_MAX &&
      heartRate >= HR_NORMAL_MIN && heartRate <= HR_NORMAL_MAX) {
    // Confidence is higher when values are in the middle of range
    float temp_center = (TEMP_NORMAL_MIN + TEMP_NORMAL_MAX) / 2.0f;
    float hr_center = (HR_NORMAL_MIN + HR_NORMAL_MAX) / 2.0f;
    
    float temp_dist = abs(bodyTemp - temp_center) / (TEMP_NORMAL_MAX - temp_center);
    float hr_dist = abs(heartRate - hr_center) / (hr_center - HR_NORMAL_MIN);
    
    *confidence = (uint8_t)(100.0f - (temp_dist + hr_dist) * 25.0f);
    *confidence = constrain(*confidence, 75, 100);
    return HEALTH_HEALTHY;
  }
  
  // Rule 4: ABNORMAL - Everything else
  // Low temp, very high HR, or inconsistent values
  if (bodyTemp < TEMP_NORMAL_MIN || heartRate < HR_NORMAL_MIN || heartRate > HR_VERY_HIGH) {
    // Lower confidence for edge cases
    *confidence = 60;
  } else {
    // Medium confidence for borderline cases
    *confidence = 70;
  }
  
  return HEALTH_ABNORMAL;
}

// Helper functions
const char* getHealthLabelVN(uint8_t healthClass) {
  if (healthClass >= 4) return "N/A";
  return HEALTH_LABELS_VN[healthClass];
}

const char* getHealthLabelEN(uint8_t healthClass) {
  if (healthClass >= 4) return "N/A";
  return HEALTH_LABELS_EN[healthClass];
}

uint16_t getHealthColor(uint8_t healthClass) {
  if (healthClass >= 4) return 0x8410;
  return HEALTH_COLORS[healthClass];
}

// Print health status to Serial
void printHealthStatus(float bodyTemp, float heartRate) {
  uint8_t confidence;
  uint8_t healthClass = classifyHealthStatus(bodyTemp, heartRate, &confidence);
  
  Serial.print("[HEALTH] ");
  Serial.print(getHealthLabelEN(healthClass));
  Serial.print(" (");
  Serial.print(confidence);
  Serial.print("%) - Temp: ");
  Serial.print(bodyTemp, 1);
  Serial.print("C, HR: ");
  Serial.print(heartRate, 0);
  Serial.println(" BPM");
}

#endif // SIMPLE_HEALTH_CLASSIFIER_H