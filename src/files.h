#pragma once

#include <string>
#include <vector>

extern std::vector<std::string> libraryPaths;

struct ID3v1Tag {
    bool exists = false;

    char title[30];
    char artist[30];
    char album[30];
    char year[4];
};

struct ID3v2Tag {
    bool exists = false;

    char identifier[3];
    char major[1];
    char minor[1];
    char flags[1];
    char size[4];

    std::string TIT2; // title
    std::string TPE1; // artists
    std::string TPE2; // album artist
    std::string TALB; // album
    std::string TYER; // year
    std::string TRCK; // track number
    std::string USLT; // unsynced lyrics

    std::vector<char> APIC; // album cover art
};

struct song {
    char path[256];
    ID3v1Tag id3v1tag;
    ID3v2Tag id3v21tag;
    
    song() {}
    song(const char *p) {
        strncpy(path, p, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    };
};

void libraryScan(const std::string path);