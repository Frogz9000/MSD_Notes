#include <Wire.h>
#include "Adafruit_MPRLS.h"

// === Actuator + Pressure Sensor Setup ===
#define LAC_PWM_PIN 5
#define RESET_PIN  -1
#define EOC_PIN    -1
#define HPA_TO_MMHG 0.75006157584566

Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

// === Pressure Points and PWM Displacements ===
float pressureSteps[] = {
  120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109,
  108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96,
  95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80
};

int pwmLevels[] = {
  155, 152, 150, 148, 146, 144, 142, 140, 138, 136,  // 120–111
  134, 132, 130, 128, 126, 124, 122, 120, 118, 116,  // 110–101
  100, 104, 108, 112, 116, 120, 124, 128,            // 100–93 (MAP)
  130, 132, 134, 136, 140, 142, 144, 146, 150, 152, 154 // 92–80
};

const int numSteps = sizeof(pressureSteps) / sizeof(pressureSteps[0]);

// === State Tracking ===
float baseline_mmHg = 0.0;
bool hasPassed130 = false;
bool oscillationStarted = false;
int stepIndex = 0;
float systolic = 0.0;
float diastolic = 0.0;

void setup() {
  Serial.begin(115200);
  pinMode(LAC_PWM_PIN, OUTPUT);
  analogWrite(LAC_PWM_PIN, 255);  // Fully extended
  delay(1000);

  Serial.println("Initializing MPRLS sensor...");
  if (!mpr.begin()) {
    Serial.println("Sensor init failed.");
    while (1);
  }

  // Record baseline
  delay(1000);
  float pressure_hPa = mpr.readPressure();
  baseline_mmHg = pressure_hPa * HPA_TO_MMHG;

  Serial.print("Baseline Pressure Set (mmHg): ");
  Serial.println(baseline_mmHg, 2);
  Serial.println("Waiting for pressure >130 mmHg to begin...");
}

void loop() {
  float current_mmHg = mpr.readPressure() * HPA_TO_MMHG;
  float relative_mmHg = round(current_mmHg - baseline_mmHg);

  Serial.print("Relative Pressure (mmHg): ");
  Serial.println(relative_mmHg);

  // Wait for user to pump to ≥130 mmHg
  if (!hasPassed130 && relative_mmHg >= 130.0) {
    hasPassed130 = true;
    Serial.println("Pressure ≥130 mmHg reached. Waiting for drop to ≤120 mmHg to begin oscillations...");
    return;
  }

  // Wait for pressure to drop to ≤120 mmHg to start
  if (hasPassed130 && !oscillationStarted && relative_mmHg <= 120.0) {
    oscillationStarted = true;
    Serial.println("Pressure dropped to ≤120 mmHg. Starting oscillations...");
    return;
  }

  // Perform oscillations
  if (oscillationStarted) {
    for (int i = stepIndex; i < numSteps; i++) {
      float pressureTarget = pressureSteps[i];

      // Allow ±1 mmHg tolerance
      if (abs(relative_mmHg - pressureTarget) <= 1) {
        int pwm = pwmLevels[i];

        // Store systolic at first match
        if (systolic == 0.0) {
          systolic = relative_mmHg;
        }

        // Always update diastolic
        diastolic = relative_mmHg;

        Serial.print("Oscillating at ");
        Serial.print(relative_mmHg);
        Serial.print(" mmHg → PWM Retract: ");
        Serial.println(pwm);

        analogWrite(LAC_PWM_PIN, pwm);   // Retract
        delay(450);
        analogWrite(LAC_PWM_PIN, 255);   // Extend
        delay(450);

        stepIndex = i + 1;
        break;  // Only one oscillation per loop
      }
    }

    // Done with all steps or pressure too low
    if (stepIndex >= numSteps || relative_mmHg <= 80.0) {
      Serial.println("Oscillations complete.");
      Serial.print("Systolic (First): ");
      Serial.println(systolic, 1);
      Serial.print("Diastolic (Last): ");
      Serial.println(diastolic, 1);
      analogWrite(LAC_PWM_PIN, 0);

      // Reset for next cycle
      Serial.println("=== READY FOR NEXT CYCLE ===");
      Serial.println("Please pump to ≥130 mmHg to restart...");

      hasPassed130 = false;
      oscillationStarted = false;
      stepIndex = 0;
      systolic = 0.0;
      diastolic = 0.0;

      delay(1000);
    }
  }

  delay(100);  // Pressure polling interval
}
