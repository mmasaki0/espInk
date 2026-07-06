#include <GxEPD2_BW.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#define EPD_BUSY 22
#define EPD_RST 20
#define EPD_DC 32
#define EPD_CS 33

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setupDisplay() {
    display.init(115200);
}

void displayWriteText(char text[]) {
    int16_t x1, y1;
    uint16_t w, h;

    display.setRotation(2);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    display.getTextBounds(text, 100, 100, &x1, &y1, &w, &h);

    display.setPartialWindow(x1, y1, w, h);

    display.firstPage();
        do {
        display.setCursor(100, 100);
        display.print(text);
        } while (display.nextPage()); // Automatically renders and flushes over SPI
    display.hibernate();
}