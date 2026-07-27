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

extern File currentFile;

#define FORMAT_LITTLEFS_IF_FAILED true

// WebServer server(80);


void setup() {
  Serial.begin(115200);
  SPI.begin(5, 21, 19, 27);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  setupDisplay();
  setupPipeline();

  if(!SD.begin(27)) {
    Serial.println("Error during SD.");
    return;
  }

  libraryScan("/lib");

  
  setupA2DP();
  displayWriteText("bluetooth connected", 10, 10);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  futureAdd(2, "front");
  futureAdd(3, "front");
  futureAdd(5, "front");
}

char qmsg[64] = "PLAYER:";

void loop() {
  // Serial.println(streamProcessed.available());
  // strcpy(qmsg, "PLAYER:");
  // xQueueSend(queueDisplay, qmsg, 0);
  // vTaskDelay(10000);
  // strcpy(qmsg, "MENU");
  // xQueueSend(queueDisplay, qmsg, 0);
  // vTaskDelay(10000);
  //   strcpy(qmsg, "MENU:");
  // xQueueSend(queueDisplay, qmsg, 0);
  // vTaskDelay(10000);
}