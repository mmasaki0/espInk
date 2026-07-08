#pragma once

#include <map>
#include <AudioTools.h>

#include <SD.h>

int32_t a2dpAudioCallback(uint8_t* data, int32_t size);
void setupPipeline();
void setupA2DP();
void sdTraverse(const char* path);

void futureAdd(uint16_t id, std::string loc);

void taskAudio(void *param);

struct song {
    char path[256];
    song() {}
    song(const char *p) {
        strncpy(path, p, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    };
};

extern std::map<uint16_t, song> mapLibrary;
extern StreamCopy copySongToPipeline;
extern QueueStream<uint8_t> streamProcessed;
extern File currentFile;

extern bool playing;
