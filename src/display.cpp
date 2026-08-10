#include <string>
#include <atomic>
#include <map>
#include <tuple>

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>


#include "bitmaps.h"
#include "unifont_latincjk.h"


#define EPD_BUSY 37
#define EPD_RST 27
#define EPD_DC 33
#define EPD_CS 32

static TaskHandle_t taskHandleDisplay = NULL;
QueueHandle_t queueDisplay;

std::atomic_int displayPlayer{0};
std::atomic_int displayMenu{0};

SemaphoreHandle_t mutexSelect;
uint8_t playerSelectIndex = 0;
uint8_t menuSelectIndex = 0;

uint16_t colorFront = GxEPD_BLACK;
uint16_t colorBack = GxEPD_WHITE;
// uint16_t colorFront = GxEPD_WHITE;
// uint16_t colorBack = GxEPD_BLACK;

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
U8G2_FOR_ADAFRUIT_GFX u8g2;

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
                    display.fillScreen(colorFront);
                    display.drawXBitmap(40, 200, bitmap::bitmap_player, 160, 64, colorBack);
                    display.drawRect(std::get<0>(playerSelectPos[playerSelectIndex]), std::get<1>(playerSelectPos[playerSelectIndex]), 32, 32, colorBack);
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
                    display.fillScreen(colorBack);
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
                    display.fillScreen(colorFront);
                    // display.drawFastHLine(0, 100, 1, GxEPD_BLACK);
                } while (display.nextPage());
            }

            strcpy(prevMsg, msg);
            lastRefresh = xTaskGetTickCount();
            refreshCounter++;
            Serial.println(lastRefresh); Serial.println(refreshCounter);
        } else {
            // refresh screen during screen downtime
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
    u8g2.begin(display);
    u8g2.setFont(unifont_latincjk);
    u8g2.setFontMode(1);
    u8g2.setForegroundColor(colorFront);
    // display.setFont(&FreeSerif9pt7b);
    display.setTextWrap(false);

    

    display.setFullWindow();
    display.firstPage();
    // display.setPartialWindow(0, 50, 240, 240);
    do {
        // display.drawXBitmap(0, 50, bitmap::arcane, 240, 240, GxEPD_BLACK);
        u8g2.setCursor(10, 120);
        u8g2.drawUTF8(10, 120, "こんにちは안녕하세요你好");
        u8g2.drawUTF8(10, 136, "GIVĒON");
    } while (display.nextPage());
    
}



void displayWriteText(std::string text, int x, int y) {
    int16_t x1, y1;
    uint16_t w, h;

    

    display.getTextBounds(text.c_str(), x, y, &x1, &y1, &w, &h);

    display.setPartialWindow(x1, y1, w, h);

    display.firstPage();
    do {
    display.setCursor(x, y);
    display.fillScreen(colorBack);
    display.print(text.c_str());
    } while (display.nextPage()); // Automatically renders and flushes over SPI
}

