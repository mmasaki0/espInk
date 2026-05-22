#include <Arduino.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <BluetoothSerial.h>
#include <BluetoothA2DPSource.h>

#include <Wifi.h>
#include <WebServer.h>
#include <config.h>

#define EPD_BUSY 4
#define EPD_RST 16
#define EPD_DC 17
#define EPD_CS 5

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

BluetoothSerial SerialBT;
BluetoothA2DPSource a2dp;

void btDeviceFound(BTAdvertisedDevice *device) {
  Serial.printf(device->getName().c_str());
}

void setup() {
  Serial.begin(115200);

  SerialBT.begin("ESP32", true);

  WiFi.begin(SSID, Password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(WiFi.localIP());


  //bluetooth scan
  if(SerialBT.discoverAsync(btDeviceFound, 10000)) {
    Serial.printf("started scanning");
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

  display.getTextBounds("yo", 100, 100, &x1, &y1, &w, &h);

  display.setPartialWindow(x1, y1, w, h);

  display.firstPage();
    do {
      display.setCursor(100, 100);
      display.print("yo");
    } while (display.nextPage()); // Automatically renders and flushes over SPI
  display.hibernate();
}

void loop() {
}
