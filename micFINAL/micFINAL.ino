/*
RSA Project: Speaker with Reactive Audio Interface
Julian Garcia, Miles Liu, Jane Yi
12/21/2020

Device overview and functional summary in final report
 */



#include <Servo.h> //include the libraries needed. Servo to control the continuous servo, and EEPROM & EEWrap for calibration
#include <EEPROM.h>
#include <EEWrap.h>
#include <Adafruit_GFX.h> // Required libraries for controlling the LED matrix, using Adafruit LED control libraries
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "RGB.h" // Extra c++ .h document with a struct to store values of common colors and their RGB values

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

//creation of an LED matrix object for use, through the Adafruit libraries.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN, // 8x8 matrix
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT + // TOP or BOTTOM ; LEFT or RIGHT 
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG, // ROWS or COLUMNS ; ZIGZAG
  NEO_GRB            + NEO_KHZ800);

void setup() {
  Serial.begin(9600); //Initialize Serial for calibration steps if need be.
  /*
   * if the #define variable designates that calibration is true,
   * calibrate for the default ambient noise values and the max sound volumes, done through helper functions (below).
   * As a summary, the user will first keep quiet to measure ambient noise. Then when prompted, the user will play music 
   * at the max expected value to let the mic to calculate an expected range of volume values.
   */
  if (calibrate) { 
    calibAmb(); 
    calibMax(); 
  }
  matrix.begin(); //initialize the created LED matrix instance
  matrix.setBrightness(10); //Set the default brightness of the LED array, accepts a range of 0 - 255
  delay(1000); //slight delay from setup to loop
}

/*
 * After setup, the code will continuously read from the output of the mic and update the volume global variable.
 * The mic will continuously read values at a rate of 200Hz for 300ms, while an array of max values read 
 * (in the form of max difference from the calibrated ambient values) are kept and updated for each 500ms period.
 * After the 500ms period, we calculate an average of the max differences array, and use that to map it to the 
 * calibrated ambient and max volume values to get the current volume level.
 */
void loop() {
  unsigned long initT = millis(); //create a variable for loop start time, used to time loop run time
  int numVals = 3; //set the number of max difference values that are kept. 
                   //The lower the value, the more reactive the system is to volume, but also generates more noise. 
  int maxDiffs[numVals]; //instantiate the max difference array
  for (int i = 0; i < numVals; i++) { //intialize the array to 0's
    maxDiffs[i] = 0;
  }
  while (millis() < initT + refreshPeriodMS) { //continuously read values from the mic and update the array for 500ms
    int a = readMic() - ambient; //for each reading, calculate the difference between the read value and ambient value
                                 //uses the readMic function (below), which limits the frequency of the mic readings
    if (a < 0) { //perform an absolute value flip
      a = a * -1;
    }
    //in the for block below, check if the value just read is within the top [numVals] values being read in this 500ms
    //period. The maxDiffs array is kept as an ordered list of those values, from largest to [numVals]'th largest.
    //We check the value just read against each stored value. If the value is greater than an element of the array,
    //everything in the array after and including that element will be shifted back by one space to make space for
    //the new value.
    for (int i = 0; i < numVals; i++) {
      if (a > maxDiffs[i]) {
        for (int j = numVals-2; j >= i; j--) {
          maxDiffs[j + 1] = maxDiffs[j];
        }
        maxDiffs[i]=a;
        break;
      }
    }
  }
  //After each 500ms period, we calculate the average of the elements in the maxDiffs array
  int diff = 0;
  for (int i = 0; i < numVals; i++) {
    diff += maxDiffs[i];
  }
  diff= diff / numVals;
  //end average difference value calculation. Output stored in diff variable
  volume = map(diff, 0, maxDiff, 0, 10); //map the diff variable from 0-10, based on the max difference value obtained
                                         //from the calibration step.
  if (volume > 10) { volume = 10; } //cap the volume variable at 10.
  if (volume > 2) { //if the volume detected is above 2, then start the servo motor. Use the Servo library for this.
                    //we use attach and detach here because the speed control was not working.
    servo.attach(servoPin);
    servo.write(0);
  }
  else {
    servo.detach(); //If the volume detected is 0 or 1, detach the motor to turn it off.
  }
  rings(volume); //update the pattern displayed on the LED array. Function is below.
}

//helper function for calibrating the ambient noise value.
void calibAmb() {
  Serial.println("Calibrating"); //Use serial throughout the function to give user queues.
  ambient = 0; //reset the EEPROM stored ambient value to 0.
  unsigned long total = 0; //initialize a sum value and a sample count for average calculation
  int count = 0;
  unsigned long initT = millis();
  //take samples for a number of seconds (definited by #define variable)
  while (millis() < initT + calibrationTimeSec * 1000) {
    total += readMic();
    count++;
  }
  ambient = total / count; //calculate the average from the sum and sample count, set the EEPROM value.
  Serial.print("Calibration done, ambient level: ");
  Serial.println(ambient); //Serial output the calculated average
}

//helper function for calibrating the max expected volume difference compared to the ambient value.
//This is done by scanning for readings with the largest magnitude difference from the ambient value,
//and setting that as the max value.
void calibMax() {
  int a = 0; //Initialize a temp variable for the maxDiff, to reduce the number of EEPROM writes.
  Serial.println("Starting max volume calibration in 5 seconds");
  delay(5000); //give the user a queue to start music at max volume
  Serial.println("Starting max volume calibration");
  unsigned long initT = millis();
  //take samples for a number of seconds (definited by #define variable). Calculate the absolute value
  //of difference between the mic reading and the stored ambient value. If the magnitude of the difference
  //is greater than the value stored, update that value.
  while (millis() < initT + calibrationTimeSec * 1000) {
    int diff = readMic() - ambient;
    if (diff < 0) {
      diff = diff * -1;
    }
    if (diff > a) {
       if (diff > 40) {
        a = 40;
      } else {
        a = diff;
      }
    }
  }
  maxDiff = a; //update the EEPROM value with the new value.
  Serial.print("Max volume calibration done, max diff: ");
  Serial.println(maxDiff); //Serial output the largest difference value.
}

//helper function to read values from the mic. This function just makes sure that the the mic is read every 
//[samplePeriodMS] amount of time (5 ms). Returns the mic reading value.
int readMic() {
  unsigned long initT = millis();
  int val = analogRead(micPin);
  while(millis() < initT + samplePeriodMS){};
  return val;
}

// Alternating "rings" for the LED array. The function takes a volume level (0-10)
// and distributes various patterns to the LED array based on that volume number.
void rings(int n) { 
  //The LED array defaults to being completely off, if the volume level is 0 or 1
  matrix.fillScreen(matrix.Color(off.r, off.g, off.b));

  //based on the input volume value, change the pattern. The color values are based off
  //of color RGB values defined in the RGB.h structs.
  switch(n) {
    case 2:
    case 3: 
      matrix.drawRect(3, 3, 2, 2, matrix.Color(blue.r, blue.g, blue.b));
      break;
    case 4:
      crossFade(one,six,5,10);
      break;
    case 5:
      colorWipe(red,0);
      break;
    case 6:
      matrix.drawRect(2, 2, 4, 4, matrix.Color(red.r, red.g, red.b));
      matrix.drawRect(3, 3, 2, 2, matrix.Color(blue.r, blue.g, blue.b));
      break;
    case 7:
      crossFade(seven,nine,5,10);
      break;
    case 8:
      matrix.drawRect(1, 1, 6, 6, matrix.Color(blue.r, blue.g, blue.b));
      matrix.drawRect(2, 2, 4, 4, matrix.Color(red.r, red.g, red.b));
      matrix.drawRect(3, 3, 2, 2, matrix.Color(blue.r, blue.g, blue.b));
      break;
    case 9:
      colorWipe(blue,0);
      break;
    case 10:
      matrix.drawRect(0, 0, 8, 8, matrix.Color(red.r, red.g, red.b));
      matrix.drawRect(1, 1, 6, 6, matrix.Color(blue.r, blue.g, blue.b));
      matrix.drawRect(2, 2, 4, 4, matrix.Color(red.r, red.g, red.b));
      matrix.drawRect(3, 3, 2, 2, matrix.Color(blue.r, blue.g, blue.b));
      break;
  }
  //apply and display the new pattern.
  matrix.show();
}

//LED display pattern for a color wipe. The speed of the pattern display can be adjusted.
void colorWipe(RGB color, uint8_t wait) {
  for(uint16_t row=0; row < 8; row++) {
    for(uint16_t column=0; column < 8; column++) {
      matrix.drawPixel(column, row, matrix.Color(color.r, color.g, color.b));
      matrix.show();
      delay(wait);
    }
  }
}

//LED display function that fades a pixel from one color to another by incrementally approaching the new color
//from the old color. The speed and number of increments for this function can be adjusted.
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

//LED display function similar to the fadePixel function, but for the entire matrix
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
