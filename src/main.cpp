#include <Arduino.h>
#include <SPI.h>

#include <SD.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <BluetoothSerial.h>
#include <BluetoothA2DPSource.h>

#include <Wifi.h>
#include <WebServer.h>
#include "FS.h"
#include <LittleFS.h>
#include <config.h>

#define EPD_BUSY 4
#define EPD_RST 16
#define EPD_DC 17
#define EPD_CS 5

#define FORMAT_LITTLEFS_IF_FAILED true

WebServer server(80);

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));



BluetoothSerial SerialBT;
BluetoothA2DPSource a2dp;

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, 27);
  if(!SD.begin(27)) {
    Serial.println("Error during SD.");
    return;
  }


  

  //begins file system, stops setup() if fails
  if(!LittleFS.begin(true)) {
    Serial.println("Error during LittleFS mount.");
    return;
  }

  SerialBT.begin("ESP32", true);

  //wifi and webserver 
  WiFi.begin(SSID, Password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  const String localIP = WiFi.localIP().toString();

  server.on("/", []() {
    Serial.println("connected to webserver");
    if(LittleFS.exists("/index.html")) {
      Serial.println("found index");
      File fileIndex = LittleFS.open("/index.html", "r");
      server.streamFile(fileIndex, "text/html");
      fileIndex.close();
    } else {
      Serial.println("did not find index");
    }
  });

  server.onNotFound([]() {
    Serial.println("webserver not found");
    server.send(404, "text/plain", "Not found");
  });

  server.begin();

  //bluetooth scan
  if(SerialBT.discoverAsync([](BTAdvertisedDevice *device) {Serial.printf(device->getName().c_str());})) {
    Serial.printf("started scanning");
    delay(10000);
    SerialBT.discoverAsyncStop();
    Serial.printf("stopped scanning");
  }

  

  //eink display
  display.init(115200);
  delay(1000);

  int16_t x1, y1;
  uint16_t w, h;

  display.setRotation(2);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);

  display.getTextBounds(localIP, 100, 100, &x1, &y1, &w, &h);

  display.setPartialWindow(x1, y1, w, h);

  display.firstPage();
    do {
      display.setCursor(100, 100);
      display.println(localIP);
    } while (display.nextPage());

  String songsList = "";

  File dirLibrary = SD.open("/library/ARIRANG");
  while(true) {
    File entry = dirLibrary.openNextFile();
    if(!entry) {
      break;
    }
    songsList += entry.name();
  }


  display.getTextBounds(songsList, 100, 120, &x1, &y1, &w, &h);
  display.setPartialWindow(x1, y1, w, h);
  display.firstPage();
    do {
      display.setCursor(100, 120);
      display.println(songsList);
    } while (display.nextPage());

  display.hibernate();
}



void loop() {
  server.handleClient();
}
