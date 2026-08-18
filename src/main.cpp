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

uint8_t buttonOnePin = 34;
uint8_t buttonTwoPin = 39;
uint8_t rotaryAPin = 26;
uint8_t rotaryBPin = 25;

#define FORMAT_LITTLEFS_IF_FAILED true

// WebServer server(80);

static TaskHandle_t taskHandleInput = NULL;

void taskInput(void* param) {
  size_t buttonOneTick = 0;
  int buttonOneCurr;
  int buttonOnePrev;

  size_t buttonTwoTick = 0;
  int buttonTwoCurr;
  int buttonTwoPrev;

  size_t rotaryTick = 0;
  int rotaryAPrev;
  int rotaryACurr;
  int rotaryBCurr;

  int debounceDelay = 50;

  while(true) {
    //button 1
    if(xTaskGetTickCount() >= buttonOneTick + debounceDelay) {
      buttonOneCurr = digitalRead(buttonOnePin);
      if(!buttonOneCurr && (buttonOneCurr != buttonOnePrev)) {
        //button 1 pressed
        Serial.println("button 1 pressed");
        buttonOneTick = xTaskGetTickCount();
        
      }
      buttonOnePrev = buttonOneCurr;
    }

    //button 2
    if(xTaskGetTickCount() >= buttonTwoTick + debounceDelay) {
      buttonTwoCurr = digitalRead(buttonTwoPin);
      if(!buttonTwoCurr && (buttonTwoCurr != buttonTwoPrev)) {
        //button 1 pressed
        Serial.println("button 2 pressed");
        buttonTwoTick = xTaskGetTickCount();
        
      }
      buttonTwoPrev = buttonTwoCurr;
    }

    //rotary encoder
    if(xTaskGetTickCount() >= rotaryTick + debounceDelay) {
      rotaryACurr = digitalRead(rotaryAPin);
      rotaryBCurr = digitalRead(rotaryBPin);

      if(!rotaryACurr && (rotaryACurr != rotaryAPrev)) {
        if(rotaryBCurr) {
          Serial.println("rotary cw");
        } else if(!rotaryBCurr) {
          Serial.println("rotary ccw");
        }
        rotaryTick = xTaskGetTickCount();
      }

      rotaryAPrev = rotaryACurr;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  SPI.begin(5, 21, 19, 4);

  vTaskDelay(2000 / portTICK_PERIOD_MS);

  pinMode(buttonOnePin, INPUT);
  pinMode(buttonTwoPin, INPUT);
  pinMode(rotaryAPin, INPUT_PULLUP);
  pinMode(rotaryBPin, INPUT_PULLUP);

  setupDisplay();

  xTaskCreatePinnedToCore(taskInput, "taskInput", 1024*3, NULL, 12, &taskHandleInput, 1);

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

