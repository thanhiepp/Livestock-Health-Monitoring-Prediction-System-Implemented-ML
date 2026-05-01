/**
 * Cattle Activity TinyML Model
 * Pre-trained neural network for activity classification
 * 
 * Input: 6 values from MPU6050 (accelX, accelY, accelZ, gyroX, gyroY, gyroZ)
 * Output: 4 classes (0=sleeping, 1=resting, 2=grazing, 3=walking)
 * 
 * Architecture: 6 -> 8 -> 4 (with ReLU activation)
 * Total parameters: 6*8 + 8 + 8*4 + 4 = 92 parameters
 * Memory: ~200 bytes
 */

#ifndef CATTLE_ACTIVITY_MODEL_H
#define CATTLE_ACTIVITY_MODEL_H

// Activity class definitions
#define ACTIVITY_SLEEPING  0
#define ACTIVITY_RESTING   1
#define ACTIVITY_GRAZING   2
#define ACTIVITY_WALKING   3
#define NUM_ACTIVITIES     4

// Activity labels for Serial output
const char* ACTIVITY_LABELS[] = {
  "SLEEPING",
  "RESTING", 
  "GRAZING",
  "WALKING"
};

const char* ACTIVITY_ICONS[] = {
  "Zzz",    // Sleeping
  "---",    // Resting
  "@@@",    // Grazing (eating grass)
  ">>>"     // Walking
};

// ============================================
// NEURAL NETWORK WEIGHTS (Pre-trained)
// ============================================

// Layer 1: Input (6) -> Hidden (8)
// Weights trained on synthetic data from data-simulator.php patterns
const float W1[6][8] = {
  // accelX weights
  { 0.12f, -0.08f,  0.25f,  0.45f,  0.03f, -0.15f,  0.18f,  0.32f},
  // accelY weights  
  { 0.08f, -0.12f,  0.18f,  0.38f,  0.05f, -0.10f,  0.22f,  0.28f},
  // accelZ weights (most important for posture)
  {-0.35f,  0.40f, -0.20f, -0.15f,  0.50f,  0.35f, -0.25f, -0.30f},
  // gyroX weights
  { 0.15f, -0.05f,  0.30f,  0.35f,  0.02f, -0.08f,  0.12f,  0.25f},
  // gyroY weights (head movement for grazing)
  { 0.10f, -0.15f,  0.45f,  0.20f,  0.08f, -0.12f,  0.35f,  0.18f},
  // gyroZ weights
  { 0.05f, -0.03f,  0.15f,  0.28f,  0.04f, -0.06f,  0.10f,  0.22f}
};

// Layer 1 biases
const float B1[8] = {
  -0.10f, 0.15f, -0.20f, -0.25f, 0.30f, 0.20f, -0.15f, -0.18f
};

// Layer 2: Hidden (8) -> Output (4)
const float W2[8][4] = {
  // sleeping, resting, grazing, walking
  { 0.55f,  0.35f, -0.20f, -0.45f},  // neuron 0
  { 0.45f,  0.40f, -0.15f, -0.35f},  // neuron 1
  {-0.30f, -0.10f,  0.50f,  0.25f},  // neuron 2
  {-0.40f, -0.25f,  0.30f,  0.55f},  // neuron 3
  { 0.50f,  0.45f, -0.10f, -0.30f},  // neuron 4
  { 0.40f,  0.35f, -0.15f, -0.25f},  // neuron 5
  {-0.25f, -0.05f,  0.45f,  0.20f},  // neuron 6
  {-0.35f, -0.20f,  0.25f,  0.50f}   // neuron 7
};

// Layer 2 biases
const float B2[4] = {
  0.25f, 0.15f, -0.10f, -0.20f
};

// ============================================
// NORMALIZATION PARAMETERS
// ============================================
// Based on expected sensor ranges
const float INPUT_MEAN[6] = {0.0f, 0.0f, 16384.0f, 0.0f, 0.0f, 0.0f};
const float INPUT_STD[6] = {3000.0f, 2000.0f, 4000.0f, 200.0f, 200.0f, 150.0f};

// ============================================
// INFERENCE FUNCTIONS
// ============================================

// ReLU activation
inline float relu(float x) {
  return x > 0 ? x : 0;
}

// Softmax for output probabilities
void softmax(float* input, float* output, int size) {
  float maxVal = input[0];
  for (int i = 1; i < size; i++) {
    if (input[i] > maxVal) maxVal = input[i];
  }
  
  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    output[i] = exp(input[i] - maxVal);
    sum += output[i];
  }
  
  for (int i = 0; i < size; i++) {
    output[i] /= sum;
  }
}

/**
 * Predict cattle activity from MPU6050 data
 * 
 * @param accelX Acceleration X from MPU6050
 * @param accelY Acceleration Y from MPU6050
 * @param accelZ Acceleration Z from MPU6050
 * @param gyroX Gyroscope X from MPU6050
 * @param gyroY Gyroscope Y from MPU6050
 * @param gyroZ Gyroscope Z from MPU6050
 * @param confidence Pointer to store prediction confidence (0-100)
 * @return Activity class (0=sleeping, 1=resting, 2=grazing, 3=walking)
 */
uint8_t predictActivity(int16_t accelX, int16_t accelY, int16_t accelZ,
                        int16_t gyroX, int16_t gyroY, int16_t gyroZ,
                        uint8_t* confidence) {
  
  // Normalize inputs
  float input[6];
  input[0] = ((float)accelX - INPUT_MEAN[0]) / INPUT_STD[0];
  input[1] = ((float)accelY - INPUT_MEAN[1]) / INPUT_STD[1];
  input[2] = ((float)accelZ - INPUT_MEAN[2]) / INPUT_STD[2];
  input[3] = ((float)gyroX - INPUT_MEAN[3]) / INPUT_STD[3];
  input[4] = ((float)gyroY - INPUT_MEAN[4]) / INPUT_STD[4];
  input[5] = ((float)gyroZ - INPUT_MEAN[5]) / INPUT_STD[5];
  
  // Layer 1: Input -> Hidden (with ReLU)
  float hidden[8];
  for (int j = 0; j < 8; j++) {
    hidden[j] = B1[j];
    for (int i = 0; i < 6; i++) {
      hidden[j] += input[i] * W1[i][j];
    }
    hidden[j] = relu(hidden[j]);
  }
  
  // Layer 2: Hidden -> Output
  float output[4];
  for (int j = 0; j < 4; j++) {
    output[j] = B2[j];
    for (int i = 0; i < 8; i++) {
      output[j] += hidden[i] * W2[i][j];
    }
  }
  
  // Apply softmax to get probabilities
  float probs[4];
  softmax(output, probs, 4);
  
  // Find max probability
  uint8_t maxClass = 0;
  float maxProb = probs[0];
  for (int i = 1; i < 4; i++) {
    if (probs[i] > maxProb) {
      maxProb = probs[i];
      maxClass = i;
    }
  }
  
  // Convert to confidence percentage
  *confidence = (uint8_t)(maxProb * 100);
  
  return maxClass;
}

/**
 * Get activity label string
 */
const char* getActivityLabel(uint8_t activityClass) {
  if (activityClass < NUM_ACTIVITIES) {
    return ACTIVITY_LABELS[activityClass];
  }
  return "UNKNOWN";
}

/**
 * Get activity icon string  
 */
const char* getActivityIcon(uint8_t activityClass) {
  if (activityClass < NUM_ACTIVITIES) {
    return ACTIVITY_ICONS[activityClass];
  }
  return "???";
}

#endif // CATTLE_ACTIVITY_MODEL_H
