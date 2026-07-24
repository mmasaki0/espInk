#include <unordered_map>

#include <SD.h>

#include "files.h"

std::unordered_map<uint16_t, song> mapLibrary;

uint16_t currentId = 0;

// rewrite this in the future
void sdTraverse(const char* path) {
    File dir = SD.open(path);
    File next = dir.openNextFile();
    while(next) {
        if(next.isDirectory()) {
            char newpath[256];
            snprintf(newpath, sizeof(newpath), "%s/%s", path, next.name());
            sdTraverse(newpath);
        } else {
            char extension[4];
            strncpy(extension, next.name() + strlen(next.name()) - 3, 3);
            extension[3] = '\0';
            if(strcmp(extension, "mp3") == 0) {
                Serial.println(next.name());
                mapLibrary[currentId++] = song(next.path());
            }
        }
        next.close();
        next = dir.openNextFile();
    }
    next.close();
    dir.close();
}