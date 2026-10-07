#define SPO2Choice  6
int value;


// Pins to control the LED signals
const int pwmRed = 3;   // PWM for red LED
const int pwmIR  = 5;   // PWM for infrared LED


// Single analog pin for reading the light input
const int lightInput = A1;


// Waveform generation variables
float angle = 0.0;
float angleStep = 0.05; // Lower value = slower waveform (controls BPM)


void setup() {
  Serial.begin(57600); // Serial MUST be this rate (or at least be the same as all the other Arduinos)
  while (!Serial) {
        ;
  }
  Serial.println("SPO2 Initialized");


  pinMode(SPO2Choice, INPUT);
  pinMode(pwmRed, OUTPUT);
  pinMode(pwmIR, OUTPUT);
}


void loop() {
  value = digitalRead(SPO2Choice);
  Serial.print("Reading: ");
  Serial.println(value);


  if (value == 1) {
        Serial.println("Choice was high");
        runSPO2Trial();  // SPO2 module conducts "normal" trial
  }
  /*
  else if (value == 0) {
        Serial.println("Choice was low");
        runSPO2Trial();  // SPO2 module conducts "normal" trial
  }
  */


  delay(2000); // Delay for testing purposes
}


void runSPO2Trial() {
  unsigned long startTime = millis();
  while (millis() - startTime < 60000) { // Run for 1 minute (60000 ms)
        int rawSignal = analogRead(lightInput); // 0–1023
        int basePWM = map(rawSignal, 0, 1023, 0, 127); // Base level


        float pulse = sin(angle); // -1.0 to 1.0
        int modulation = int(pulse * 127.5 + 127.5); // Convert to 0–255


        int pwmValueRed = constrain(basePWM + modulation / 2, 0, 255);
        int pwmValueIR  = constrain(basePWM + modulation / 2 * 0.95, 0, 255); // Slight variation


        analogWrite(pwmRed, pwmValueRed);
        analogWrite(pwmIR, pwmValueIR);


        angle += angleStep;
        if (angle > TWO_PI) angle -= TWO_PI;


        delay(20); // Update frequency
  }


  // Turn off LEDs after trial
  analogWrite(pwmRed, 0);
  analogWrite(pwmIR, 0);
}
