"""
Human Activity Recognition Model Training Script
Train a neural network model for Arduino/TinyML deployment

Requirements:
    pip install numpy pandas scikit-learn tensorflow

Usage:
    python train_activity_model.py activity_data.csv
"""

import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import tensorflow as tf
from tensorflow import keras
import sys
import os

# Activity labels
ACTIVITIES = ['LYING', 'SITTING', 'STANDING', 'WALKING', 'RUNNING']
NUM_ACTIVITIES = 5

def load_data(filepath):
    """Load and preprocess the collected data"""
    print(f"\n{'='*60}")
    print(f"Loading data from: {filepath}")
    print(f"{'='*60}")
    
    # Load CSV
    df = pd.read_csv(filepath)
    print(f"Total samples loaded: {len(df)}")
    
    # Check for missing values
    if df.isnull().sum().sum() > 0:
        print("⚠️  Warning: Found missing values, dropping them...")
        df = df.dropna()
        print(f"Samples after cleaning: {len(df)}")
    
    # Print distribution
    print("\nActivity distribution:")
    for i in range(NUM_ACTIVITIES):
        count = len(df[df['activity'] == i])
        print(f"  {ACTIVITIES[i]:12} : {count:5} samples ({count/len(df)*100:.1f}%)")
    
    # Separate features and labels
    X = df[['accelX', 'accelY', 'accelZ', 'gyroX', 'gyroY', 'gyroZ']].values
    y = df['activity'].values
    
    return X, y

def normalize_data(X_train, X_test):
    """Normalize the data using StandardScaler"""
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    
    print("\nNormalization parameters:")
    print("Mean:", scaler.mean_)
    print("Std: ", scaler.scale_)
    
    return X_train_scaled, X_test_scaled, scaler

def create_model(input_dim=6, hidden1=12, hidden2=8, output_dim=5):
    """Create neural network model"""
    model = keras.Sequential([
        keras.layers.Dense(hidden1, activation='relu', input_dim=input_dim),
        keras.layers.Dense(hidden2, activation='relu'),
        keras.layers.Dense(output_dim, activation='softmax')
    ])
    
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    return model

def train_model(model, X_train, y_train, X_test, y_test, epochs=50):
    """Train the model"""
    print(f"\n{'='*60}")
    print(f"Training model for {epochs} epochs...")
    print(f"{'='*60}")
    
    history = model.fit(
        X_train, y_train,
        epochs=epochs,
        batch_size=32,
        validation_data=(X_test, y_test),
        verbose=1
    )
    
    return history

def evaluate_model(model, X_test, y_test):
    """Evaluate model performance"""
    print(f"\n{'='*60}")
    print("Model Evaluation")
    print(f"{'='*60}")
    
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
    print(f"Test Accuracy: {test_acc*100:.2f}%")
    print(f"Test Loss: {test_loss:.4f}")
    
    # Per-class accuracy
    y_pred = np.argmax(model.predict(X_test, verbose=0), axis=1)
    
    print("\nPer-class accuracy:")
    for i in range(NUM_ACTIVITIES):
        mask = y_test == i
        if mask.sum() > 0:
            acc = (y_pred[mask] == y_test[mask]).mean()
            print(f"  {ACTIVITIES[i]:12} : {acc*100:.2f}%")

def export_weights_to_c(model, scaler, output_file='model_weights.h'):
    """Export model weights to C header file for Arduino"""
    print(f"\n{'='*60}")
    print(f"Exporting weights to: {output_file}")
    print(f"{'='*60}")
    
    weights = model.get_weights()
    
    with open(output_file, 'w') as f:
        f.write("// Auto-generated model weights\n")
        f.write("// DO NOT EDIT MANUALLY\n\n")
        
        # Layer 1: Input -> Hidden1
        f.write("// Layer 1: Input (6) -> Hidden1 (12)\n")
        f.write("const float W1[6][12] = {\n")
        W1 = weights[0]
        for i in range(6):
            f.write("  {")
            for j in range(12):
                f.write(f"{W1[i,j]:8.5f}f")
                if j < 11:
                    f.write(", ")
            f.write("}")
            if i < 5:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")
        
        f.write("const float B1_MODEL[12] = {\n  ")
        B1 = weights[1]
        for i in range(12):
            f.write(f"{B1[i]:8.5f}f")
            if i < 11:
                f.write(", ")
        f.write("\n};\n\n")
        
        # Layer 2: Hidden1 -> Hidden2
        f.write("// Layer 2: Hidden1 (12) -> Hidden2 (8)\n")
        f.write("const float W2[12][8] = {\n")
        W2 = weights[2]
        for i in range(12):
            f.write("  {")
            for j in range(8):
                f.write(f"{W2[i,j]:8.5f}f")
                if j < 7:
                    f.write(", ")
            f.write("}")
            if i < 11:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")
        
        f.write("const float B2[8] = {\n  ")
        B2 = weights[3]
        for i in range(8):
            f.write(f"{B2[i]:8.5f}f")
            if i < 7:
                f.write(", ")
        f.write("\n};\n\n")
        
        # Layer 3: Hidden2 -> Output
        f.write("// Layer 3: Hidden2 (8) -> Output (5)\n")
        f.write("const float W3[8][5] = {\n")
        W3 = weights[4]
        for i in range(8):
            f.write("  {")
            for j in range(5):
                f.write(f"{W3[i,j]:8.5f}f")
                if j < 4:
                    f.write(", ")
            f.write("}")
            if i < 7:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")
        
        f.write("const float B3[5] = {\n  ")
        B3 = weights[5]
        for i in range(5):
            f.write(f"{B3[i]:8.5f}f")
            if i < 4:
                f.write(", ")
        f.write("\n};\n\n")
        
        # Normalization parameters
        f.write("// Normalization parameters\n")
        f.write("const float INPUT_MEAN[6] = {\n  ")
        for i in range(6):
            f.write(f"{scaler.mean_[i]:10.2f}f")
            if i < 5:
                f.write(", ")
        f.write("\n};\n\n")
        
        f.write("const float INPUT_STD[6] = {\n  ")
        for i in range(6):
            f.write(f"{scaler.scale_[i]:10.2f}f")
            if i < 5:
                f.write(", ")
        f.write("\n};\n")
    
    print(f"✅ Weights exported successfully!")
    print(f"📝 Copy the contents of {output_file} to human_activity_model.h")

def main():
    if len(sys.argv) < 2:
        print("Usage: python train_activity_model.py <data_file.csv>")
        print("\nExample:")
        print("  python train_activity_model.py activity_data.csv")
        sys.exit(1)
    
    data_file = sys.argv[1]
    
    if not os.path.exists(data_file):
        print(f"Error: File '{data_file}' not found!")
        sys.exit(1)
    
    # Load data
    X, y = load_data(data_file)
    
    # Split data
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    print(f"\nTrain samples: {len(X_train)}")
    print(f"Test samples:  {len(X_test)}")
    
    # Normalize
    X_train_scaled, X_test_scaled, scaler = normalize_data(X_train, X_test)
    
    # Create model
    model = create_model()
    print("\nModel architecture:")
    model.summary()
    
    # Train
    history = train_model(model, X_train_scaled, y_train, X_test_scaled, y_test)
    
    # Evaluate
    evaluate_model(model, X_test_scaled, y_test)
    
    # Export weights
    export_weights_to_c(model, scaler)
    
    # Save full model
    model.save('human_activity_model.h5')
    print(f"\n✅ Full model saved to: human_activity_model.h5")
    
    print(f"\n{'='*60}")
    print("🎉 Training complete!")
    print(f"{'='*60}")
    print("\nNext steps:")
    print("1. Copy weights from model_weights.h to human_activity_model.h")
    print("2. Upload the updated code to your Arduino")
    print("3. Test the model with real-time data")
    print("\nNote: You may need to adjust the architecture or collect more data")
    print("      if the accuracy is too low.")

if __name__ == "__main__":
    main()

"""
TIPS FOR BETTER ACCURACY:

1. DATA COLLECTION:
   - Collect at least 500-1000 samples per activity
   - Collect data from multiple people
   - Collect data in different environments
   - Ensure consistent sensor orientation

2. DATA QUALITY:
   - Remove noisy samples
   - Ensure balanced dataset (equal samples per class)
   - Add data augmentation if needed

3. MODEL TUNING:
   - Try different architectures (more/fewer neurons)
   - Adjust learning rate
   - Increase training epochs
   - Use dropout layers to prevent overfitting

4. TESTING:
   - Test on real device, not just simulation
   - Test with different people
   - Test edge cases (transitions between activities)
   - Monitor confidence scores

5. OPTIMIZATION:
   - If accuracy is low on device, recalibrate gyro
   - Adjust normalization parameters
   - Consider using more features (magnitude, variance, etc.)
"""