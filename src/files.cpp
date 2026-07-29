#include <vector>
#include <string>
#include <algorithm>

#include <SD.h>

#include "files.h"

SemaphoreHandle_t mutexCurrentFile;

std::vector<std::string> libraryPaths;

File currentFile;
song currentSong;

// rewrite this in the future
void libraryScan(const std::string path) {
    File dir = SD.open(path.c_str());
    File next = dir.openNextFile();
    while(next) {
        if(next.isDirectory()) {
            libraryScan(next.path());
        } else {
            char extension[4];
            strncpy(extension, next.name() + strlen(next.name()) - 3, 3);
            extension[3] = '\0';
            if(strcmp(extension, "mp3") == 0) {
                Serial.println(next.path());
                libraryPaths.push_back(next.path());
            }
        }
        next.close();
        next = dir.openNextFile();
    }
    next.close();
    dir.close();
    
    std::sort(libraryPaths.begin(), libraryPaths.end());
}