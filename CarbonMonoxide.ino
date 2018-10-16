/*
*/
#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>

int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor
int prevCOReading = 0;
int COReadingCount = 0; 
float overall = 0;

void setup() {
  Serial.begin(113200);
  pinMode(sensorPin, INPUT);
  prevCOReading = analogRead(sensorPin);

  Wire.begin();       // serial communication with OLED display
  oled.init();        // Initialze SSD1306 OLED display
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Glucose Meter");
}

float readCO(){
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Measuring CO...");
  
  float average = 0;
  int sum = 0;
  for(int i=0; i<50; i++)
  {
    sensorValue = analogRead(sensorPin);
    sum = sum + sensorValue;
    delay(100);
    Serial.print(".");
    oled.setTextXY(1,0);              // Set cursor position, start of line 1
    if(i%4 == 0){
      oled.putString("-");  
    }
    if(i%4 == 1){
      oled.putString("\\");  
    }
    if(i%4 == 2){
      oled.putString("|");  
    }
    if(i%4 == 3){
      oled.putString("/");  
    }
    
  }
  Serial.println("");
  average = sum / 50.0;
  overall = overall + sum;
  return average;
}

void writeOLED(float sensorVal05Sec, float sensorVal10Sec, float sensorVal15Sec, float sensorVal20Sec){
  char str[4][10];
  sprintf(str[0], "05s %4.0f", sensorVal05Sec);
  sprintf(str[1], "10s %4.0f", sensorVal10Sec);
  sprintf(str[2], "15s %4.0f", sensorVal15Sec);
  sprintf(str[3], "20s %4.0f", sensorVal20Sec);
  
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("CO Values:");
  oled.setTextXY(1,0);              // Set cursor position, start of line 1
  oled.putString(str[0]);
  oled.setTextXY(2,0);              // Set cursor position, start of line 2
  oled.putString(str[1]);
  oled.setTextXY(3,0);              // Set cursor position, start of line 3
  oled.putString(str[2]);
  oled.setTextXY(4,0);              // Set cursor position, start of line 4
  oled.putString(str[3]);
}

void loop() {
  float sensorVal05Sec, sensorVal10Sec, sensorVal15Sec, sensorVal20Sec;  
  char current[16];
  
  sensorValue = analogRead(sensorPin);
  COReadingCount++;

  if (COReadingCount > 50) {
    Serial.printf("Prev = %d, Current = %d Diff = %d\n", prevCOReading, sensorValue, (sensorValue - prevCOReading));
    sprintf(current, "Current: %5d", sensorValue);
    oled.setTextXY(6,0);              // Set cursor position, start of line 4
    oled.putString(current);
    overall = 0;
    if((sensorValue - prevCOReading) > 40)
    { 
      Serial.println("Reading CO value:");
      /* ten second delay for stable reading */
      for(int i=0;i<10;i++) {
        Serial.print(".");
        delay(1000);
      }
      Serial.println("");
      sensorVal05Sec = readCO();
      Serial.printf("\nAverage after 5 sec = %.0f\n", sensorVal05Sec);
      sensorVal10Sec = readCO();
      Serial.printf("\nAverage after 10 sec = %.0f\n", sensorVal10Sec);
      sensorVal15Sec = readCO();
      Serial.printf("\nAverage after 15 sec = %.0f\n", sensorVal15Sec);
      sensorVal20Sec = readCO();
      Serial.printf("\nAverage after 20 sec = %.0f\n", sensorVal20Sec);
      Serial.printf("Summary: %.0f %.0f %.0f %.0f\n", sensorVal05Sec, sensorVal10Sec, sensorVal15Sec, sensorVal20Sec);
      Serial.printf("Overall: %.0f\n", overall/200);
      writeOLED(sensorVal05Sec, sensorVal10Sec, sensorVal15Sec, sensorVal20Sec);
    }
    
    prevCOReading = sensorValue;
    COReadingCount = 0;
  }
  delay(100);
}
