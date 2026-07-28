#pragma once

#include <atomic>

#include <SD.h>

#include <AudioTools.h>

int32_t a2dpAudioCallback(uint8_t* data, int32_t size);
void preSetup();
void setupPipeline();
void setupA2DP();

void changeCurrentFile(const std::string path);
void futureAdd(uint16_t id, std::string loc);

void taskAudio(void *param);


extern StreamCopy copySongToPipeline;
extern QueueStream<uint8_t> streamProcessed;
extern MeasuringStream measure;
extern File currentFile;

extern std::atomic_int playing;
