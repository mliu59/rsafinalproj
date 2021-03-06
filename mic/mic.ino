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
#define samplePeriodMS 5 //the period of each sample read from the microphone, 40Hz
#define refreshPeriodMS 500 //the length of the period in which volume levels are read and updated, 0.5Hz
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

boolean flip = true;

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
  int numVals = 5;
  int maxDiffs[numVals];
  for (int i = 0; i < numVals; i++) {
    maxDiffs[i] = 0;
  }
  while (millis() < initT + refreshPeriodMS) {
    int a = readMic() - ambient;
    if (a < 0) {
      a = a * -1;
    }
    for (int i = 0; i < numVals; i++) {
      if (a > maxDiffs[i]) {
        for (int j = numVals - 2; j >= i; j--) {
          maxDiffs[j + 1] = maxDiffs[j];
        }
        maxDiffs[i] = a; 
        break;
      }
    }
  }
  for (int i = 0; i < numVals; i++) {
    diff += maxDiffs[i];
  }
  diff = diff / numVals;
  volume = map(diff, 0, maxDiff, 0, 10);
  if (volume > 10) { volume = 10; }
  Serial.println(volume);
  if (volume > 2) {
    servo.write(0);
  }

  switch(volume) {
    case 0:
      colorWipe(off, 0);
      break;
    case 1:
      colorWipe(off, 0);
      break;
    case 2:
      //colorWipe(two, 0);
      crossFade(one, two, 5, 10);
      break;
    case 3:
      //colorWipe(three, 0);
      crossFade(two, three, 5, 10);
      break;
    case 4:
      //colorWipe(four, 0);
      crossFade(three, four, 5, 10);
      break;
    case 5:
      //colorWipe(five, 0);
      crossFade(four, five, 5, 10);
      break;
    case 6:
      //colorWipe(six, 0);
      crossFade(five, six, 5, 10);
      break;
    case 7:
      //colorWipe(seven, 0);
      crossFade(six, seven, 5, 10);
      break;
    case 8:
      //colorWipe(eight, 0);
      crossFade(seven, eight, 5, 10);
      break;
    case 9:
      //colorWipe(nine, 0);
      crossFade(eight, nine, 5, 10);
      break;
    case 10:
      //colorWipe(red, 0);
      crossFade(nine, red, 5, 10);
      break;
  }
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
      if (diff > 50) {
        maxDiff = 50;
      } else {
        maxDiff = diff;
      }
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
