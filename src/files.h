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

extern std::vector<std::string, AllocatorSTLPSRAM<std::string>> libraryPaths;
extern SemaphoreHandle_t mutexCurrentFile;

struct ID3v1Tag {
    bool exists = false;

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

    std::vector<char, AllocatorSTLPSRAM<char>> TIT2; // title
    std::vector<char, AllocatorSTLPSRAM<char>> TPE1; // artists
    std::vector<char, AllocatorSTLPSRAM<char>> TPE2; // album artist
    std::vector<char, AllocatorSTLPSRAM<char>> TALB; // album
    std::vector<char, AllocatorSTLPSRAM<char>> TYER; // year
    std::vector<char, AllocatorSTLPSRAM<char>> TRCK; // track number
    std::vector<char, AllocatorSTLPSRAM<char>> USLT; // unsynced lyrics
    std::vector<char, AllocatorSTLPSRAM<char>> APIC; // album cover art
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
            char header[10]; //begin use as id3v2 header
            // std::vector<char, AllocatorSTLPSRAM<char>> frameData;

            file.readBytes(header, 10);

            if(memcmp(header, "ID3", 3) == 0) {
                //has ID3v2 tag
                id3v2.exists = true;
                // Serial.print((uint8_t)header[0]); Serial.print(" "); Serial.print((uint8_t)header[1]); Serial.print(" "); Serial.print((uint8_t)header[2]); Serial.print(" "); Serial.print((uint8_t)header[3]); Serial.print(" "); Serial.print((uint8_t)header[4]); Serial.print(" "); Serial.print((uint8_t)header[5]); Serial.print(" "); Serial.print((uint8_t)header[6]); Serial.print(" "); Serial.print((uint8_t)header[7]); Serial.print(" "); Serial.print((uint8_t)header[8]); Serial.print(" "); Serial.println((uint8_t)header[9]);
                id3v2.size = (header[6] << 21) | (header[7] << 14) | (header[8] << 7) | header[9];
                // Serial.println(id3v2.size);
                
                //repurpose header for frame headers
                while(file.position() < id3v2.size) {
                    // Serial.println(file.position());
                    size_t bytesRead = file.readBytes(header, 10);
                    if(bytesRead == 10) {
                        Serial.print(header[0]); Serial.print(header[1]); Serial.print(header[2]); Serial.print(header[3]);
                        int frameSize = (header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7];   
                        Serial.println(frameSize);

                        if(frameSize <= 1024 * 8) {
                            // read data into memory if of interest

                            if(memcmp(header, "TIT2", 4) == 0) {
                                id3v2.TIT2.resize(frameSize);
                                file.readBytes(id3v2.TIT2.data(), frameSize);
                                Serial.println("copied TIT2");
                            } else if(memcmp(header, "TPE1", 4) == 0) {
                                id3v2.TPE1.resize(frameSize);
                                file.readBytes(id3v2.TPE1.data(), frameSize);
                                Serial.println("copied TPE1");
                            } else if(memcmp(header, "TPE2", 4) == 0) {
                                id3v2.TPE2.resize(frameSize);
                                file.readBytes(id3v2.TPE2.data(), frameSize);
                                Serial.println("copied TPE2");
                            } else if(memcmp(header, "TALB", 4) == 0) {
                                id3v2.TALB.resize(frameSize);
                                file.readBytes(id3v2.TALB.data(), frameSize);
                                Serial.println("copied TALB");
                            } else if(memcmp(header, "TYER", 4) == 0) {
                                id3v2.TYER.resize(frameSize);
                                file.readBytes(id3v2.TYER.data(), frameSize);
                                Serial.println("copied TYER");
                            } else if(memcmp(header, "TRCK", 4) == 0) {
                                id3v2.TRCK.resize(frameSize);
                                file.readBytes(id3v2.TRCK.data(), frameSize);
                                Serial.println("copied TRCK");
                            } else if(memcmp(header, "USLT", 4) == 0) {
                                id3v2.USLT.resize(frameSize);
                                file.readBytes(id3v2.USLT.data(), frameSize);
                                Serial.println("copied USLT");
                            } else if(memcmp(header, "APIC", 4) == 0) {
                                id3v2.APIC.resize(frameSize);
                                file.readBytes(id3v2.APIC.data(), frameSize);
                                Serial.println("copied APIC");
                            } else {
                                //skip
                                if(frameSize == 0) {
                                    file.seek(file.size());
                                } else {
                                    file.seek(file.position() + frameSize);
                                }
                                
                            }
                        } else {
                            //skip
                            file.seek(file.position() + frameSize);
                        }

                    } else {
                        // stop reading
                        file.seek(file.size());
                        Serial.println("stop");
                    }
                }
                    
                    

                //     // char tag[5];
                //     // memcpy(tag, buffer.data(), 4);
                //     // tag[4] = '\0';
                //     // Serial.println(tag);

                //     i += frameSize + 10;
                //     file.seek(file.position() + frameSize + 10);
                // }

            }
        }


        file.close();
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

extern TaskHandle_t taskHandleFileData;

void taskFileData(void *param);

void setupLibrary();