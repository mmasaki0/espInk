#include <Arduino.h>
#include <SPI.h>

#include <SD.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <BluetoothSerial.h>
#include <BluetoothA2DPSource.h>

// #include <Wifi.h>
// #include <WebServer.h>
// #include "FS.h"
// #include <LittleFS.h>
#include <config.h>

#include <AudioTools.h>
#include <AudioTools/Communication/A2DPStream.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Concurrency/RTOS.h>

#define EPD_BUSY 4
#define EPD_RST 16
#define EPD_DC 17
#define EPD_CS 5

#define FORMAT_LITTLEFS_IF_FAILED true

// WebServer server(80);

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));



BluetoothSerial SerialBT;
BluetoothA2DPSource a2dp_source;

BufferRTOS<uint8_t> bufferProcessed(1024 * 8);
QueueStream<uint8_t> queueProcessed(bufferProcessed);

File song1;

//song -> queueEncoded -> decoder -> resampler -> queueProcessed
ResampleStream resampler(queueProcessed);
EncodedAudioStream decoder(&song1, new MP3DecoderHelix());
StreamCopy copierPipelineToQueue(queueProcessed, decoder);

Task taskScreen("screen", 1024 * 2, 5, 0);
Task taskPipelineToQueue("PipelineToQueue", 1024 * 2, 10, 1);

int32_t get_sound_data(uint8_t* data, int32_t size) {
  int32_t result = queueProcessed.readBytes((uint8_t*)data, size);
  vTaskDelay(pdMS_TO_TICKS(1));
  return(result);
}

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, 27);
  if(!SD.begin(27)) {
    Serial.println("Error during SD.");
    return;
  }

  Serial.println("Starting audio");
  song1 = SD.open("/library/ARIRANG/SWIM.mp3");

  Serial.println("Starting decoder");
  decoder.transformationReader().resizeResultQueue(1024 * 6);
  if (!decoder.begin()) {
    Serial.println("decoder failed");
    stop();
  }

  // Serial.println("Starting resampler");
  // auto rcfg = resampler.defaultConfig();
  // rcfg.copyFrom(decoder.audioInfo());
  // rcfg.sample_rate = 44100;
  // resampler.begin(rcfg);

  Serial.println("Starting queues");
  queueProcessed.begin();

  //audio pipeline
  Serial.println("starting audio pipeline task");
  
  taskPipelineToQueue.begin([](){
    if (queueProcessed.availableForWrite() > 4608) {
      copierPipelineToQueue.copy();
    } else {
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  });

  Serial.println((double)decoder.audioInfo().sample_rate);

  Serial.println("Starting bluetooth");
  a2dp_source.set_data_callback(get_sound_data);
  a2dp_source.start("hachiware - Find My");

  while(!a2dp_source.is_connected()) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial.print(".");
  }

  // //begins file system, stops setup() if fails
  // if(!LittleFS.begin(true)) {
  //   Serial.println("Error during LittleFS mount.");
  //   return;
  // }

  // SerialBT.begin("ESP32", true);

  // //wifi and webserver 
  // WiFi.begin(SSID, Password);
  // while(WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  // const String localIP = WiFi.localIP().toString();

  // server.on("/", []() {
  //   Serial.println("connected to webserver");
  //   if(LittleFS.exists("/index.html")) {
  //     Serial.println("found index");
  //     File fileIndex = LittleFS.open("/index.html", "r");
  //     server.streamFile(fileIndex, "text/html");
  //     fileIndex.close();
  //   } else {
  //     Serial.println("did not find index");
  //   }
  // });

  // server.onNotFound([]() {
  //   Serial.println("webserver not found");
  //   server.send(404, "text/plain", "Not found");
  // });

  // server.begin();

  //bluetooth scan
  // if(SerialBT.discoverAsync([](BTAdvertisedDevice *device) {Serial.printf(device->getName().c_str());})) {
  //   Serial.printf("started scanning");
  //   delay(10000);
  //   SerialBT.discoverAsyncStop();
  //   Serial.printf("stopped scanning");
  // }

  //eink display
  // display.init(115200);
  // delay(1000);
  // display.hibernate();
 

}

  // int16_t x1, y1;
  // uint16_t w, h;

  // display.setRotation(2);
  // display.setTextColor(GxEPD_BLACK);
  // display.setFont(&FreeMonoBold9pt7b);

  // display.getTextBounds(localIP, 100, 100, &x1, &y1, &w, &h);

  // display.setPartialWindow(x1, y1, w, h);

  // display.firstPage();
  //   do {
  //     display.setCursor(100, 100);
  //     display.println(localIP);
  //   } while (display.nextPage());

  // String songsList = "";

  // File dirLibrary = SD.open("/library/ARIRANG");
  // while(true) {
  //   File entry = dirLibrary.openNextFile();
  //   if(!entry) {
  //     break;
  //   }
  //   songsList += entry.name();
  // }


  // display.getTextBounds(songsList, 100, 120, &x1, &y1, &w, &h);
  // display.setPartialWindow(x1, y1, w, h);
  // display.firstPage();
  //   do {
  //     display.setCursor(100, 120);
  //     display.println(songsList);
  //   } while (display.nextPage());





void loop() {
  // server.handleClient();
}
