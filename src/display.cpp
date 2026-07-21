#include <string>
#include <atomic>
#include <map>
#include <tuple>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>

#include "bitmaps.h"

#define EPD_BUSY 22
#define EPD_RST 20
#define EPD_DC 32
#define EPD_CS 33

static TaskHandle_t taskHandleDisplay = NULL;
QueueHandle_t queueDisplay;

std::atomic_int displayPlayer{0};
std::atomic_int displayMenu{0};

SemaphoreHandle_t mutexSelect;
uint8_t playerSelectIndex = 0;
uint8_t menuSelectIndex = 0;

//maps selection index to screen position
std::map<uint8_t, std::tuple<uint16_t, uint16_t>> playerSelectPos = {
    {0, {104, 200}},
    {1, {136, 200}},
    {2, {168, 200}},
    {3, {136, 232}},
    {4, {104, 232}},
    {5, {72, 232}},
    {6, {40, 200}},
    {7, {72, 200}}
};

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));


void taskDisplay(void *param) {
    int refreshCounter = 0;
    TickType_t lastRefresh = 0;
    char prevMsg[64];
    while(1) {
        //recieve message in queue
        char msg[64];
        if(xQueueReceive(queueDisplay, (void*)&msg, 0) == pdTRUE) {

            //player screen
            if(strncmp(msg, "PLAYER:", 7) == 0) {
                Serial.println("top");
                display.setPartialWindow(40, 200, 160, 64);
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_WHITE);
                    display.drawXBitmap(40, 200, bitmap_player, 160, 64, GxEPD_BLACK);
                    display.drawRect(std::get<0>(playerSelectPos[playerSelectIndex]), std::get<1>(playerSelectPos[playerSelectIndex]), 32, 32, GxEPD_BLACK);
                } while (display.nextPage());
                
            }

            //menu screen
            else if(strcmp(msg, "MENU") == 0) {
                Serial.println("full menu");
                // display.setPartialWindow(0, 0, display.width(), display.height());
                display.setPartialWindow(40, 200, 160, 64);
                // display.fillScreen(GxEPD_WHITE);
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_BLACK);
                    // display.drawFastHLine(0, 100, 1, GxEPD_BLACK);
                } while (display.nextPage());
            }
            else if(strcmp(msg, "MENU:") == 0) {
                Serial.println("full menu");
                // display.setPartialWindow(0, 0, display.width(), display.height());
                display.setPartialWindow(40, 200, 160, 64);
                // display.fillScreen(GxEPD_WHITE);
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_WHITE);
                    // display.drawFastHLine(0, 100, 1, GxEPD_BLACK);
                } while (display.nextPage());
            }

            strcpy(prevMsg, msg);
            lastRefresh = xTaskGetTickCount();
            refreshCounter++;
            Serial.println(lastRefresh); Serial.println(refreshCounter);
        } else {
            // Serial.println("pdfalse");
            if(refreshCounter > 2 && xTaskGetTickCount() > lastRefresh + 5000) {
                Serial.println(lastRefresh); Serial.println(refreshCounter);
                display.setFullWindow();
                display.refresh();
                lastRefresh = xTaskGetTickCount();
                refreshCounter = 0;
            }
        }
        vTaskDelay(pdTICKS_TO_MS(1));
    }
    vTaskDelete(NULL);
}

void setupDisplay() {
    xTaskCreatePinnedToCore(taskDisplay, "taskDisplay", 1024*3, NULL, 1, &taskHandleDisplay, 1);
    queueDisplay = xQueueCreate(4, 64);
    mutexSelect = xSemaphoreCreateMutex();

    display.init(115200);
    display.setRotation(2);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSerif9pt7b);
}



void displayWriteText(std::string text) {
    int16_t x1, y1;
    uint16_t w, h;

    

    display.getTextBounds(text.c_str(), 100, 100, &x1, &y1, &w, &h);

    display.setPartialWindow(x1, y1, w, h);

    display.firstPage();
        do {
        display.setCursor(100, 100);
        display.fillScreen(GxEPD_WHITE);
        display.print(text.c_str());
        } while (display.nextPage()); // Automatically renders and flushes over SPI
    display.hibernate();
}

