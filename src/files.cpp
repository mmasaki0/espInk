#include <vector>
#include <string>
#include <array>
#include <algorithm>

#include <SD.h>

#include "files.h"

SemaphoreHandle_t mutexCurrentFile;

std::vector<std::array<char, 256>, AllocatorSTLPSRAM<std::array<char, 256>>> libraryPaths;

File currentFile;
// song currentSong;

// recursive directory scan
void libraryScan(const char* path, int depth = 0) {
    File dir = SD.open(path);
    File next = dir.openNextFile();
    while(next) {
        if(strlen(next.path()) < 256) {
            if(next.isDirectory()) {
                if(depth < 3) {libraryScan(next.path(), depth + 1);}
            } else {
                const char* name = next.name();
                size_t len = strlen(name);
                if(len >= 4 && strcasecmp(name + len - 4, ".mp3") == 0) {
                    std::array<char, 256> entry{};
                    strcpy(entry.data(), next.path());
                    libraryPaths.push_back(entry);
                    Serial.println(entry.data());
                }
            }
        }
        next.close();
        next = dir.openNextFile();
    }
    dir.close();
}

void setupLibrary() {
    libraryPaths.clear();
    libraryScan("/lib");
    std::sort(libraryPaths.begin(), libraryPaths.end());
}