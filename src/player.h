#pragma once

#include <map>
#include <AudioTools.h>

int32_t a2dpAudioCallback(uint8_t* data, int32_t size);
void setupPipeline();
void setupA2DP();
void sdTraverse(const char* path);

struct song {
    char path[256];
    song() {}
    song(const char *p) {
        strncpy(path, p, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    };
};

extern std::map<uint64_t, song> mapLibrary;
extern StreamCopy copySongToPipeline;


