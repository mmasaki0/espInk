#include <Arduino.h>
#include <SPI.h>

#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"
// put function declarations here:

void setup() {
   pinMode(4, INPUT);  //BUSY
   pinMode(16, OUTPUT); //RES 
   pinMode(17, OUTPUT); //DC   
   pinMode(5, OUTPUT); //CS

   //SPI
   SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0)); 
   SPI.begin(18, -1, 23, 5);

   Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  // digitalWrite(14, LOW);
  // delay(2000);
  // digitalWrite(14, HIGH);
  // delay(2000);
}
