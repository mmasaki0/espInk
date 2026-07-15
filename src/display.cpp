#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>

#include "bitmaps.h"

#include <string>

#define EPD_BUSY 22
#define EPD_RST 20
#define EPD_DC 32
#define EPD_CS 33

static TaskHandle_t taskHandleDisplay = NULL;
QueueHandle_t queueDisplay;

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));


void taskDisplay(void *param) {
    while(1) {
        //recieve message in queue
        char msg[64];
        if(xQueueReceive(queueDisplay, (void*)&msg, portMAX_DELAY) == pdTRUE) {
            if(strncmp(msg, "PLAYER:", 7) == 0) {
                Serial.println("top");
                display.setPartialWindow(40, 200, 160, 64);
                
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_WHITE);
                    display.drawXBitmap(40, 200, bitmap_player, 160, 64, GxEPD_BLACK);
                } while (display.nextPage());
                
            }
            if(strncmp(msg, "MENU:", 5) == 0) {
                Serial.println("top");
                display.setPartialWindow(40, 200, 160, 64);
                // display.fillScreen(GxEPD_WHITE);
                display.firstPage();
                do {
                    display.fillScreen(GxEPD_WHITE);
                } while (display.nextPage());
            }
            display.powerOff();
        }
        vTaskDelay(pdTICKS_TO_MS(1));
    }
    vTaskDelete(NULL);
}

void setupDisplay() {
    xTaskCreatePinnedToCore(taskDisplay, "taskDisplay", 1024*3, NULL, 1, &taskHandleDisplay, 1);
    queueDisplay = xQueueCreate(4, 64);

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

