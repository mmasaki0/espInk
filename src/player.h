#pragma once

#include <AudioTools.h>

int32_t a2dpAudioCallback(uint8_t* data, int32_t size);
void setupPipeline();
void setupA2DP();

extern StreamCopy copySongToPipeline;

