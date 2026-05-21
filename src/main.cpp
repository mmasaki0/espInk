#include <Arduino.h>
#include <SPI.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>

#define EPD_BUSY 4
#define EPD_RST 16
#define EPD_DC 17
#define EPD_CS 5

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup() {
  display.init(115200);
  display.setRotation(1);
  display.setFont(&FreeMono12pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(100,100);
  display.print("yo");
  display.display();
}

void loop() {


  esp_deep_sleep_start();
}
