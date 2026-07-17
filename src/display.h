#pragma once

#include <string>

void setupDisplay();
void displayWriteText(std::string text);

extern QueueHandle_t queueDisplay;

extern std::atomic_int displayPlayer;
extern std::atomic_int displayMenu;

extern SemaphoreHandle_t mutexSelect;
extern uint8_t playerSelectIndex;
extern uint8_t menuSelectIndex;