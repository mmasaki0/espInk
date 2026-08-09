#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <cctype>

#include <SD.h>

#include "files.h"

SemaphoreHandle_t mutexCurrentFile;

std::vector<std::string, AllocatorSTLPSRAM<std::string>> libraryPaths;

File currentFile;
song currentSong;

TaskHandle_t taskHandleFileData = NULL;

// recursive directory scan
void libraryScan(const std::string path, int depth = 0) {
    File dir = SD.open(path.c_str());
    File next = dir.openNextFile();
    while(next) {

        if(next.isDirectory()) {
            if(depth < 3) {libraryScan(static_cast<std::string>(next.path()), depth + 1);}
        } else {
            // const char* name = next.name();
            // size_t len = strlen(name);
            // if(len >= 4 && strcasecmp(name + len - 4, ".mp3") == 0) {
            //     std::array<char, 256> entry{};
            //     strcpy(entry.data(), next.path());
            //     libraryPaths.push_back(entry);
            //     Serial.println(entry.data());
            // }

            std::string name = next.name();
            size_t len = name.length();
            if(len >= 4) {
                std::string extension = name.substr(len - 4, 4);
                for(int i = 0; i < 4; i++) {
                    extension[i] = std::tolower(extension[i]);
                }
                if(extension == ".mp3") {
                    libraryPaths.push_back(next.path());
                    Serial.println(next.path());
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

void taskFileData(void *param) {
    //run once to read current file
    
    // currentSong = song( *(static_cast<std::string*>(param)) );
    // Serial.println(currentSong.id3v1.exists);

    vTaskDelete(NULL);
}