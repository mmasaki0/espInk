#pragma once

#include <string>

void setupDisplay();
void displayWriteText(std::string text);

extern QueueHandle_t queueDisplay;