#include <string>

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <AudioTools/Concurrency/RTOS.h>
// #include <Wifi.h>
// #include <WebServer.h>
// #include "FS.h"
// #include <LittleFS.h>

#include "config.h"
#include "player.h"
#include "display.h"
#include "files.h"

#define FORMAT_LITTLEFS_IF_FAILED true

// WebServer server(80);


void setup() {
  Serial.begin(115200);
  SPI.begin(5, 21, 19, 4);
  
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  setupDisplay();
  preSetup();
  setupPipeline();

  if(!SD.begin(4)) {
    Serial.println("Error during SD.");
    return;
  }

  setupLibrary();
  
  setupA2DP();
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  futureAdd(15, "front");
  futureAdd(16, "front");
  futureAdd(17, "front");
  futureAdd(18, "front");
  futureAdd(19, "front");
  futureAdd(20, "front");
  futureAdd(21, "front");
  futureAdd(22, "front");
  futureAdd(23, "front");
  Serial.print("buffer:"); Serial.println(esp_ptr_internal(&bufferProcessed));
}

char qmsg[64] = "PLAYER:";

void loop() {
  vTaskDelay(1000);
  Serial.println(bufferProcessed.levelPercent());
  Serial.println(esp_get_free_internal_heap_size());
}