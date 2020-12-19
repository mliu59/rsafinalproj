#include <Servo.h>
#include <EEPROM.h>
#include <EEWrap.h>

#define samplePeriodMS 25
#define calibrationTimeSec 10
#define micPin A1
#define servoPin 9
#define calibrate true
#define refreshPeriodMS 500

int volume = 0;
int16_e ambient EEMEM;
int16_e maxDiff EEMEM;

Servo servo;

void setup() {
  Serial.begin(9600);
  servo.attach(servoPin);
  if (calibrate) {
    calibAmb();
    calibMax();
  }
  delay(1000);
}
        
void loop() {
  unsigned long initT = millis();
  unsigned long total = 0;
  int count = 0;
  while (millis() < initT + refreshPeriodMS) {
    total += abs(readMic() - ambient);
    count++;
  }
  int diff = total / count;
  volume = map(diff, 0, maxDiff, 0, 10);
  if (volume > 10) { volume = 10; }
  Serial.println(volume);
  servo.write(volume * 18);
}


void calibAmb() {
  Serial.println("Calibrating");
  ambient = 0;
  unsigned long total = 0;
  int count = 0;
  unsigned long initT = millis();
  while (millis() < initT + calibrationTimeSec * 1000) {
    total += readMic();
    count++;
  }
  ambient = total / count;
  Serial.print("Calibration done, ambient level: ");
  Serial.println(ambient);
}

void calibMax() {
  maxDiff = 0;
  Serial.println("Starting max volume calibration in 5 seconds");
  delay(5000);
  Serial.println("Starting max volume calibration");
  unsigned long initT = millis();
  while (millis() < initT + calibrationTimeSec * 1000) {
    unsigned int diff = abs(readMic() - ambient);
    if (diff > maxDiff) {
      maxDiff = diff;
    }
  }
  Serial.print("Max volume calibration done, max diff: ");
  Serial.println(maxDiff);
}

int readMic() {
  unsigned long initT = millis();
  int val = analogRead(micPin);
  while(millis() < initT + samplePeriodMS){};
  return val;
}
