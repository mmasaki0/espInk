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
        if(n > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        if(n * sizeof(T) == 0) {
            return nullptr;
        }
        
        void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if(!ptr) {
            throw std::bad_alloc();
        }

        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, size_t) noexcept {
        heap_caps_free(p);
    }   
};
template<typename T, typename U> bool operator==(const AllocatorSTLPSRAM<T>&, const AllocatorSTLPSRAM<U>&) {return true;}
template<typename T, typename U> bool operator!=(const AllocatorSTLPSRAM<T>&, const AllocatorSTLPSRAM<U>&) {return false;}

extern std::vector<std::array<char, 256>, AllocatorSTLPSRAM<std::array<char, 256>>> libraryPaths;
extern SemaphoreHandle_t mutexCurrentFile;

struct ID3v1Tag {
    bool exists = false;;

    std::string title;
    std::string artist;
    std::string album;
    std::string year;
};

struct ID3v2Tag {
    bool exists = false;

    // char identifier[3];
    // char major[1];
    // char minor[1];
    // char flags[1];
    // char size[4];

    int size;

    std::string TIT2; // title
    std::string TPE1; // artists
    std::string TPE2; // album artist
    std::string TALB; // album
    std::string TYER; // year
    std::string TRCK; // track number
    std::string USLT; // unsynced lyrics

    std::vector<char> APIC; // album cover art
};

struct VBR {
    bool exists = false;
    uint32_t frames = 0;
    uint32_t bytes = 0;
    uint8_t toc[100];
};

struct song {
    std::string path;
    ID3v1Tag id3v1;
    ID3v2Tag id3v2;
    VBR vbr;
    
    song() {}
    song(const std::string p) {
        path = p;
        loadID3v1();
        loadID3v2();
        loadVBR();
    };

    void loadID3v1() {
        File file = SD.open(path.c_str());

        if(file.size() >= 128) {
            file.seek(file.size() - 128);
            std::vector<char> buffer;

            buffer.resize(128);
            file.readBytes(buffer.data(), 128);

            if(memcmp(buffer.data(), "TAG", 3) == 0) {
                // has ID3v1 tag
                id3v1.exists = true;

                id3v1.title = misc::trim_copy(std::string(buffer.data() + 3, 30));
                id3v1.artist = misc::trim_copy(std::string(buffer.data() + 33, 30));
                id3v1.album = misc::trim_copy(std::string(buffer.data() + 63, 30));
                id3v1.year = misc::trim_copy(std::string(buffer.data() + 93, 4));

                file.close();
            }
        }

        file.close();
        id3v1.exists = false;
    }

    void loadID3v2() {
        File file = SD.open(path.c_str());

        if(file.size() > 10) {
            std::vector<char> buffer(10);

            file.readBytes(buffer.data(), 10);

            if(memcmp(buffer.data(), "ID3", 3) == 0) {
                //has ID3v2 tag
                id3v2.exists = true;
                id3v2.size = (buffer.at(6) << 21) | (buffer.at(7) << 14) | (buffer.at(8) << 7) | buffer.at(9);

            }
        }


        file.close();
            id3v2.exists = false;
    }

    bool loadAPIC() {
        return false;
    }

    void loadVBR() {
        File file = SD.open(path.c_str());

        // if(id3v2.exists) {
        //     file.seek(10 + id3v2.sizeCalculated);
        // } else {
        //     file.seek(0);
        // }


        file.close();
        vbr.exists = false;
    }
};



extern File currentFile;
extern song currentSong;

void setupLibrary();