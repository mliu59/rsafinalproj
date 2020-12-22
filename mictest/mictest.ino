#define sampleDelay 25


void setup() {
  Serial.begin(9600);
}
        
void loop() {
  unsigned long initT = millis();
  Serial.println(analogRead(A1));
  while(millis() < initT + sampleDelay){};
}
