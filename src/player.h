#pragma once

#include <atomic>

#include <SD.h>

#include <AudioTools.h>

int32_t a2dpAudioCallback(uint8_t* data, int32_t size);
void setupPipeline();
void setupA2DP();
void sdTraverse(const char* path);

void changeCurrentFile(const char* path);
void futureAdd(uint16_t id, std::string loc);

void taskAudio(void *param);


extern StreamCopy copySongToPipeline;
extern QueueStream<uint8_t> streamProcessed;
extern File currentFile;

extern std::atomic_int playing;
