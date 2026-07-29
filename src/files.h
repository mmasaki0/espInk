#pragma once

#include <string>
#include <vector>

#include <SD.h>

#include "misc.h"

template <typename T>
struct AllocatorSTLPSRAM {
    using value_type = T;

    AllocatorSTLPSRAM() noexcept = default;

    template <typename U>
    AllocatorSTLPSRAM(const AllocatorSTLPSRAM<U>&) noexcept {}

    T* allocate(size_t n) {
        if(n > numeric_limits<size_t>::max / sizeof(T)) {
            throw bad_array_new_length();
        }

        if(n * sizeof(T) == 0) {
            return nullptr;
        }
        
        void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if(!ptr) {
            throw bad_alloc();
        }

        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, size_t) noexcept {
        heap_caps_free(p);
    }   
};
template<typename T, typename U> bool operator==(const AllocatorSTLPSRAM<T>&, const AllocatorSTLPSRAM<U>&) {return true;}
template<typename T, typename U> bool operator!=(const AllocatorSTLPSRAM<T>&, const AllocatorSTLPSRAM<U>&) {return false;}

extern std::vector<std::string> libraryPaths;
extern SemaphoreHandle_t mutexCurrentFile;

struct ID3v1Tag {
    bool exists;

    std::string title;
    std::string artist;
    std::string album;
    std::string year;
};

struct ID3v2Tag {
    bool exists;

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
    std::string path;
    ID3v1Tag id3v1tag;
    ID3v2Tag id3v2tag;
    
    song() {}
    song(const std::string p) {
        path = p;
    };

    bool loadID3v1() {
        File file = SD.open(path.c_str());

        if(file.size() >= 128) {
            file.seek(file.size() - 128);
            std::vector<char> buffer;

            buffer.resize(128);
            file.readBytes(buffer.data(), 128);

            if(memcmp(buffer.data(), "TAG", 3) == 0) {
                // has ID3v1 tag
                id3v1tag.exists = true;

                id3v1tag.title = misc::trim_copy(std::string(buffer.data() + 3, 30));
                id3v1tag.artist = misc::trim_copy(std::string(buffer.data() + 33, 30));
                id3v1tag.album = misc::trim_copy(std::string(buffer.data() + 63, 30));
                id3v1tag.year = misc::trim_copy(std::string(buffer.data() + 93, 4));

                file.close();
                return true;
            }

        }

        file.close();
        id3v1tag.exists = false;
        return false;
    }

    bool loadID3v2() {
        File file = SD.open(path.c_str());

        if(file.size() > 10) {
            std::vector<char> buffer;
            buffer.resize(10);
            file.readBytes(buffer.data(), 10);

            if(memcmp(buffer.data(), "ID3", 3) == 0) {
                //has ID3v2 tag
                id3v2tag.exists = true;

                int tagSize = 0;


            }
        }


        file.close();
            id3v2tag.exists = false;
            return false;
    }
};


extern File currentFile;
extern song currentSong;

void libraryScan(const std::string path);