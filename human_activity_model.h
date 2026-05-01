
#ifndef HUMAN_ACTIVITY_MODEL_H
#define HUMAN_ACTIVITY_MODEL_H

#include <math.h>

// Activity class definitions - 3 CLASSES
#define ACTIVITY_NGHI       0  // Rest (Lying + Sitting)
#define ACTIVITY_DUNG       1  // Standing
#define ACTIVITY_DI_CHUYEN  2  // Moving (Walking + Running)
#define NUM_ACTIVITIES      3

// Activity labels
const char* ACTIVITY_LABELS[] = {"NGHI", "DUNG", "DI_CHUYEN"};
const char* ACTIVITY_VIETNAMESE[] = {"NGHI", "DUNG", "DI CHUYEN"};

// ============================================
// NEURAL NETWORK WEIGHTS (3-class model)
// Trained with 97.46% accuracy
// ============================================

// Auto-generated 3-class model weights
// Classes: NGHI (Rest), DUNG (Standing), DI_CHUYEN (Moving)
// DO NOT EDIT MANUALLY

// Layer 1: Input (6) -> Hidden1 (16)
const float W1[6][16] = {
  { 1.16936f, -0.56625f,  0.75079f,  0.05884f, -1.43843f,  0.33584f, -0.24751f, -0.81864f, -0.17581f,  0.45971f,  0.35227f, -0.96028f, -0.21784f,  0.59737f, -0.29064f, -0.58972f},
  { 0.55521f,  0.74754f,  0.08309f, -0.01339f, -0.11392f, -0.00887f, -1.04728f,  0.34389f, -1.50268f, -0.09984f,  0.05479f, -0.29158f,  0.67923f,  0.01215f, -1.59037f, -1.03643f},
  {-0.39563f, -0.87445f, -0.85858f,  1.15812f, -0.65152f,  0.14341f,  0.35513f,  0.63184f, -0.69317f, -0.02248f,  0.77873f, -0.73155f, -1.17510f,  0.10715f, -0.70020f,  0.19018f},
  { 0.03473f, -0.12764f,  0.14673f,  0.15384f,  0.07264f,  0.00169f,  0.34077f,  0.33751f, -0.07794f,  0.06427f,  0.12641f,  0.05882f,  0.36851f, -0.72616f, -0.09427f,  0.01661f},
  {-0.03183f, -0.34269f,  0.05183f,  0.10240f, -0.06194f, -0.03729f, -0.05206f, -0.03704f,  0.07143f,  0.18414f, -0.30604f,  0.01035f,  0.51036f,  0.47291f,  0.04278f,  0.18208f},
  {-0.01143f,  0.46998f, -0.04773f,  0.31094f,  0.03872f, -1.73234f,  0.05970f,  0.30094f, -0.04699f,  1.57169f, -0.98090f, -0.04351f,  0.14272f,  0.28760f, -0.04715f,  0.23392f}
};

const float B1_MODEL[16] = {
  -0.78639f,  0.51094f, -0.27691f, -0.12813f,  0.91892f,  0.33314f, -0.23380f, -0.36739f, -0.48792f,  0.39660f,  0.20919f,  1.00242f,  0.38824f,  0.72074f, -0.41166f, -0.55906f
};

// Layer 2: Hidden1 (16) -> Hidden2 (8)
const float W2[16][8] = {
  { 1.73936f,  1.22344f,  0.34589f,  0.58360f,  0.30431f,  0.07628f, -0.95071f, -1.30973f},
  { 0.10589f,  0.22046f,  0.87940f,  0.32922f,  1.08240f,  0.50818f,  0.38535f,  0.10857f},
  { 0.38634f,  1.03270f,  0.49217f,  0.04445f,  0.76096f, -0.13207f,  0.21315f, -0.67512f},
  { 0.39639f, -0.04897f, -0.54826f,  0.13730f, -0.20490f,  0.77805f, -1.13625f, -0.23980f},
  {-0.13751f,  1.45165f,  0.20788f,  0.08816f,  1.86898f, -0.20275f,  0.77124f, -0.00376f},
  { 1.21269f,  0.04441f, -1.01209f, -1.32388f,  0.38357f,  0.38753f, -1.63490f, -0.48734f},
  {-0.02000f, -0.75613f,  0.36931f, -0.96769f, -0.33674f,  0.33910f,  0.80215f,  0.75080f},
  { 0.12703f, -1.03844f,  0.45056f, -0.48709f,  0.38227f,  0.40114f,  0.52334f,  0.68024f},
  {-0.85036f, -0.82990f,  0.87211f, -1.10562f, -0.59412f,  0.03870f,  1.49842f,  0.74420f},
  { 0.94401f,  0.01614f, -0.41786f,  0.44519f, -0.65734f,  0.40354f, -1.08641f, -0.18873f},
  { 1.52523f, -0.00248f, -0.12428f, -0.08859f,  0.45538f,  0.72193f, -0.95981f,  0.28981f},
  { 0.29107f,  1.52479f,  0.38979f,  0.32521f,  0.85715f, -0.10052f,  0.69504f,  0.56716f},
  {-0.14441f,  0.04019f,  0.44790f,  0.20238f,  0.62292f,  0.05594f,  0.11673f,  0.03361f},
  { 1.12725f,  0.14930f,  0.03338f,  0.16018f,  0.06755f,  0.66557f, -0.57985f, -0.50903f},
  {-0.11176f, -0.77979f,  1.02566f, -0.79715f, -0.02209f,  0.12420f,  0.92362f,  0.98064f},
  {-0.50706f,  0.18199f,  0.50266f,  0.90523f,  1.37941f, -0.47846f,  0.56291f,  0.47638f}
};

const float B2[8] = {
   0.81536f,  0.04032f, -0.20332f,  0.30797f,  0.12274f,  0.86049f,  0.05258f,  0.23358f
};

// Layer 3: Hidden2 (8) -> Output (3)
const float W3[8][3] = {
  {-1.91981f,  0.06783f,  0.00564f},
  { 0.86397f, -0.84893f,  1.19892f},
  { 0.71419f,  0.71030f, -0.87756f},
  { 0.11703f, -0.55394f,  1.12980f},
  {-0.15931f, -1.32660f,  0.17373f},
  {-1.33731f,  0.60829f, -0.27004f},
  { 0.17127f, -0.86451f, -1.25521f},
  { 0.85018f, -1.27408f, -0.34553f}
};

const float B3[3] = {
  -0.29693f,  0.55579f, -0.41343f
};

// Normalization parameters
const float INPUT_MEAN[6] = {
    10403.75f,   -1648.16f,   -2663.03f,     -96.97f,      -6.81f,      -4.85f
};

const float INPUT_STD[6] = {
     8142.20f,    8266.36f,    5370.02f,    3357.30f,    1497.32f,    1279.00f
};


// ============================================
// INFERENCE FUNCTIONS
// ============================================

inline float relu(float x) { return x > 0 ? x : 0; }

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
 * Predict human activity from MPU6050 data
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
  
  // Layer 1: Input (6) -> Hidden1 (16)
  float hidden1[16];
  for (int j = 0; j < 16; j++) {
    hidden1[j] = B1_MODEL[j];
    for (int i = 0; i < 6; i++) {
      hidden1[j] += input[i] * W1[i][j];
    }
    hidden1[j] = relu(hidden1[j]);
  }
  
  // Layer 2: Hidden1 (16) -> Hidden2 (8)
  float hidden2[8];
  for (int j = 0; j < 8; j++) {
    hidden2[j] = B2[j];
    for (int i = 0; i < 16; i++) {
      hidden2[j] += hidden1[i] * W2[i][j];
    }
    hidden2[j] = relu(hidden2[j]);
  }
  
  // Layer 3: Hidden2 (8) -> Output (3)
  float output[3];
  for (int j = 0; j < 3; j++) {
    output[j] = B3[j];
    for (int i = 0; i < 8; i++) {
      output[j] += hidden2[i] * W3[i][j];
    }
  }
  
  // Softmax (chinh xac nhat)
  float probs[3];
  softmax(output, probs, 3);
  
  // Find max
  uint8_t maxClass = 0;
  float maxProb = probs[0];
  for (int i = 1; i < 3; i++) {
    if (probs[i] > maxProb) {
      maxProb = probs[i];
      maxClass = i;
    }
  }
  
  *confidence = (uint8_t)(maxProb * 100);
  return maxClass;
}

const char* getActivityLabel(uint8_t activityClass) {
  return (activityClass < NUM_ACTIVITIES) ? ACTIVITY_LABELS[activityClass] : "UNKNOWN";
}

const char* getActivityLabelVN(uint8_t activityClass) {
  return (activityClass < NUM_ACTIVITIES) ? ACTIVITY_VIETNAMESE[activityClass] : "???";
}

#endif // HUMAN_ACTIVITY_MODEL_H