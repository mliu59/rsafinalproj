/*
 * 
 * 
 * 
 */



#include <Servo.h> //include the libraries needed. Servo to control the continuous servo, and EEPROM & EEWrap for calibration
#include <EEPROM.h>
#include <EEWrap.h>
#include <Adafruit_GFX.h> // Required libraries
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "RGB.h" // Extra function

#define calibrationTimeSec 10 //the duration of the calibration periods, both ambient and max volume
#define micPin A1 //pin connected to the mic output
#define servoPin 9 //pin connected to the continuous rotation servo signal output
#define samplePeriodMS 25 //the period of each sample read from the microphone, 40Hz
#define refreshPeriodMS 2000 //the length of the period in which volume levels are read and updated, 0.5Hz
#define calibrate false //this variable designates whether or not a calibration will be conducted at the beginning.
#define PIN 6 // Data pin for LED

int volume = 0; //global variable for the most recent detected volume. Values range between 0 - 10, and are updated at 0.5Hz freq
int16_e ambient EEMEM; //EEPROM variables used for calibration. This makes calibration not necessary everytime
int16_e maxDiff EEMEM;

Servo servo; //create servo object for disco ball spinning

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN, // 8x8 matrix
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT + // TOP or BOTTOM ; LEFT or RIGHT 
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG, // ROWS or COLUMNS ; ZIGZAG
  NEO_GRB            + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  servo.attach(servoPin);
  servo.write(90);
  if (calibrate) {
    calibAmb();
    calibMax();
  }
  
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(20); // 0 - 255
  matrix.setTextColor(matrix.Color(255, 255, 255)); // (R, G, B)
  
  delay(1000);
}

void loop() {
  unsigned long initT = millis();
  int diff = 0;
  while (millis() < initT + refreshPeriodMS) {
    int a = readMic() - ambient;
    if (a < 0) {
      a = a * -1;
    }
    if (a > diff) {
      diff = a;
    }
  }
  volume = map(diff, 0, maxDiff, 0, 10);
  if (volume > 10) { volume = 10; }
  Serial.println(volume);
  if (volume > 2) {
    servo.write(0);
  }

  drawLogo(volume);
  delay(500);
  
  crossFade(off, white, 50, 10 - volume);
  delay(500);

  colorWipe(red, 50 - (volume * 5));
  delay(500);
  
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
    int diff = abs(readMic() - ambient);
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

void colorWipe(RGB color, uint8_t wait) {
  for(uint16_t row=0; row < 8; row++) {
    for(uint16_t column=0; column < 8; column++) {
      matrix.drawPixel(column, row, matrix.Color(color.r, color.g, color.b));
      matrix.show();
      delay(wait);
    }
  }
}

void fadePixel(int x, int y, RGB startColor, RGB endColor, int steps, int wait) {
  for(int i = 0; i <= steps; i++) {
    int newR = startColor.r + (endColor.r - startColor.r) * i / steps;
    int newG = startColor.g + (endColor.g - startColor.g) * i / steps;
    int newB = startColor.b + (endColor.b - startColor.b) * i / steps;

    matrix.drawPixel(x, y, matrix.Color(newR, newG, newB));
    matrix.show();
    delay(wait);
  }
}

void crossFade(RGB startColor, RGB endColor, int steps, int wait) {
  for(int i = 0; i <= steps; i++) {
    int newR = startColor.r + (endColor.r - startColor.r) * i / steps;
    int newG = startColor.g + (endColor.g - startColor.g) * i / steps;
    int newB = startColor.b + (endColor.b - startColor.b) * i / steps;

    matrix.fillScreen(matrix.Color(newR, newG, newB));
    matrix.show();
    delay(wait);
  }
}

void drawLogo(int vol) {
  int logo[8][8] = {  
   {0, 0, 0, 0, 0, 0, 0, 0},
   {0, 1, 1, 0, 0, 1, 1, 0},
   {0, 1, 1, 0, 0, 1, 1, 0},
   {0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0},
   {0, 1, 1, 0, 0, 1, 1, 0},
   {0, 1, 1, 0, 0, 1, 1, 0},
   {0, 0, 0, 0, 0, 0, 0, 0}
  };

  for(int row = 0; row < 8; row++) {
    for(int column = 0; column < 8; column++) {
      if(logo[row][column] == 1) {
        fadePixel(column, row, red, white, 120, 10-vol);
      }
    }
  }
}

void scrollText(String textToDisplay) {
  int x = matrix.width();

  int pixelsInText = textToDisplay.length() * 7;

  matrix.setCursor(x, 0);
  matrix.print(textToDisplay);
  matrix.show();

  while(x > (matrix.width() - pixelsInText)) {
    matrix.fillScreen(matrix.Color(red.r, red.g, red.b));
    matrix.setCursor(--x, 0);
    matrix.print(textToDisplay);
    matrix.show();
    delay(150);
  }
}
