#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <string>

// #include <Wifi.h>
// #include <WebServer.h>
// #include "FS.h"
// #include <LittleFS.h>
#include <config.h>

#include <AudioTools/Concurrency/RTOS.h>

#include "player.h"
#include "display.h"



#define FORMAT_LITTLEFS_IF_FAILED true

// WebServer server(80);



Task taskDisplay("taskDisplay", 2048, 5, 1);
Task taskAudio("taskAudio", 1600, 20, 1);



void setup() {
  Serial.begin(115200);
  SPI.begin(5, 21, 19, 27);
  // AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if(!SD.begin(27)) {
    Serial.println("Error during SD.");
    return;
  }

  sdTraverse("/library");

  setupDisplay();

  setupPipeline();
  setupA2DP();
  displayWriteText("bluetooth connected");
  

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  addToFuture(2, "front");
  addToFuture(3, "front");
  addToFuture(5, "front");

  

  // Serial.println(esp_get_free_heap_size());
  // sdTraverse("/library");
  // Serial.println(esp_get_free_heap_size());

  // for(auto& p : mapLibrary) {
  //   Serial.println(p.first); Serial.println(p.second.path);
  // }

  taskAudio.begin([](){
    if(!playing) {
      static uint8_t zeros[512] = {0};
      streamProcessed.write(zeros, sizeof(zeros));
    } else {
      copySongToPipeline.copy();
    }

    // Serial.println(copiedBytes);

    // //check if end of song
    // if(copiedBytes == 0 && streamProcessed.available() == 0) {
    //   // Serial.println("end of song");
    //   //wip
    // }
    
    vTaskDelay(pdMS_TO_TICKS(1));
  });

  // displayWriteText(std::to_string(analogRead(35) * 2));

  // taskDisplay.begin([](){
    
  // });
  
}

void loop() {
  vTaskDelay(1000);
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
 

// }

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





// void loop() {
//   // server.handleClient();
// }
