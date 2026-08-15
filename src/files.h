#pragma once

#include <string>
#include <vector>
#include <algorithm>


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
        // loadVBR();
    };

    void loadID3v1() {
        File file = SD.open(path.c_str());

        if(file.size() < 128) {
            file.close();
            return;
        }

        file.seek(file.size() - 128);

        std::vector<char> buffer;
        buffer.reserve(128);
        file.readBytes(buffer.data(), 128);

        if(memcmp(buffer.data(), "TAG", 3) == 0) {
            // has ID3v1 tag
            id3v1.title = misc::trim_copy(std::string(buffer.data() + 3, 30));
            id3v1.artist = misc::trim_copy(std::string(buffer.data() + 33, 30));
            id3v1.album = misc::trim_copy(std::string(buffer.data() + 63, 30));
            id3v1.year = misc::trim_copy(std::string(buffer.data() + 93, 4));
        }

        file.close();
    }

    void loadID3v2() {
        File file = SD.open(path.c_str());

        if(file.size() < 10) {
            file.close();
            return;
        }

        char header[10]; //begin use as id3v2 header
        file.readBytes(header, 10);

        if(memcmp(header, "ID3", 3) != 0) {
            file.close();
            return;
        }

        //has ID3v2 tag
        id3v2.exists = true;
        // Serial.print((uint8_t)header[0]); Serial.print(" "); Serial.print((uint8_t)header[1]); Serial.print(" "); Serial.print((uint8_t)header[2]); Serial.print(" "); Serial.print((uint8_t)header[3]); Serial.print(" "); Serial.print((uint8_t)header[4]); Serial.print(" "); Serial.print((uint8_t)header[5]); Serial.print(" "); Serial.print((uint8_t)header[6]); Serial.print(" "); Serial.print((uint8_t)header[7]); Serial.print(" "); Serial.print((uint8_t)header[8]); Serial.print(" "); Serial.println((uint8_t)header[9]);
        id3v2.size = (header[6] << 21) | (header[7] << 14) | (header[8] << 7) | header[9];
        // Serial.println(id3v2.size);
        
        //repurpose header for frame headers
        while(file.position() < id3v2.size) {
            size_t bytesRead = file.readBytes(header, 10);

            if(bytesRead != 10 || header[0] == 0) {
                return;
            }
            // Serial.print(header[0]); Serial.print(header[1]); Serial.print(header[2]); Serial.print(header[3]);
            int frameSize = (header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7];   

            // Serial.println(frameSize);

            std::vector<char, AllocatorSTLPSRAM<char>>* outputPtr = nullptr;

            // check if tag is of interest and set output
            
            //limit max size of data reading
            if(memcmp(header, "TIT2", 4) == 0 ) {
                outputPtr = &id3v2.TIT2;
                outputPtr->reserve(512);
                Serial.println("pointing to TIT2");
            } else if(memcmp(header, "TPE1", 4) == 0) {
                outputPtr = &id3v2.TPE1;
                outputPtr->reserve(512);
                Serial.println("pointing to TPE1");
            } else if(memcmp(header, "TPE2", 4) == 0) {
                outputPtr = &id3v2.TPE2;
                outputPtr->reserve(512);
                Serial.println("pointing to TPE2");
            } else if(memcmp(header, "TALB", 4) == 0) {
                outputPtr = &id3v2.TALB;
                outputPtr->reserve(512);
                Serial.println("pointing to TALB");
            } else if(memcmp(header, "TYER", 4) == 0) {
                outputPtr = &id3v2.TYER;
                outputPtr->reserve(4);
                Serial.println("pointing to TYER");
            } else if(memcmp(header, "TRCK", 4) == 0) {
                outputPtr = &id3v2.TRCK;
                outputPtr->reserve(3);
                Serial.println("pointing to TRCK");
            } else if(memcmp(header, "USLT", 4) == 0) {
                outputPtr = &id3v2.USLT;
                outputPtr->reserve(8192);
                Serial.println("pointing to USLT");
            } else if(memcmp(header, "APIC", 4) == 0) {
                if(frameSize <= 8192) {
                    //for images cant truncate so only read small images
                    outputPtr = &id3v2.APIC;
                    outputPtr->reserve(8192);
                    // Serial.println("pointing to APIC");
                }
            }

            if(outputPtr == nullptr) {
                file.seek(file.position() + frameSize);
                continue;
            }

            Serial.println("not null");
            //tag is of interest, read contents
            std::vector<char> frameData;
            int frameDataCapacity = 1024*8;
            frameData.reserve(frameDataCapacity);
            int frameBytesRead = file.readBytes(frameData.data(), std::min(frameSize, frameDataCapacity)); // only read up to framedata capacity

            bool utf16Decode = false;
            bool bigEndian = true;
            int decodeIndex = 1;

            if(header[0] == 'T') {
                switch(frameData[0]) {
                    case 0x00: {
                        //ISO-8859-1 (ASCII)
                        Serial.println("case 0");
                        Serial.println(std::min(frameSize, static_cast<int>(outputPtr->capacity())));
                        memcpy(outputPtr->data(), frameData.data() + 1, std::min(frameSize - 1, static_cast<int>(outputPtr->capacity())));

                        break;
                    }
                    case 0x01: {
                        //UTF16 with BOM
                        Serial.println("case 1");
                        utf16Decode = true;

                        if(frameData.size() < 3) {
                            file.seek(file.position() + (frameSize - frameBytesRead));
                            continue;
                        }

                        if(frameData.data()[1] == 0xFE && frameData.data()[2] == 0xFF) {
                            Serial.println("manual select endian 1");
                            decodeIndex += 2;
                        } else if(frameData.data()[1] == 0xFF && frameData.data()[2] == 0xFE) {
                            Serial.println("manual select endian 2");
                            bigEndian = false;
                            decodeIndex += 2;
                        }

                        break;
                    }
                    case 0x02: {
                        //UTF16BE without BOM
                        Serial.println("case 2");
                        utf16Decode = true;

                        if(frameData.size() < 3) {
                            file.seek(file.position() + (frameSize - frameBytesRead));
                            continue;
                        }

                        if((frameData.data()[1] == 0xFE && frameData.data()[2] == 0xFF) || (frameData.data()[1] == 0xFF && frameData.data()[2] == 0xFE)) {
                            decodeIndex += 2;
                        }   

                        break;
                    }
                    // case 0x03: {
                    //     //UTF8
                    //     Serial.println("case 3");
                    //     outputPtr->assign(frameData.begin() + 1, frameData.begin() + outputPtr->capacity());

                    //     break;
                    // }
                    default: {
                        break;
                    }

                    if(utf16Decode) {
                        //decode utf16 to utf8
                        while(decodeIndex + 1 < frameSize) {
                            uint32_t codepoint;
                            uint16_t first16Unit; 

                            if(bigEndian) {
                                first16Unit = (static_cast<uint8_t>(frameData.data()[decodeIndex]) << 8) | static_cast<uint8_t>(frameData.data()[decodeIndex + 1]); 
                            } else {
                                first16Unit = static_cast<uint8_t>(frameData.data()[decodeIndex]) | (static_cast<uint8_t>(frameData.data()[decodeIndex + 1]) << 8);
                            }
                
                            // decode codepoint from utf16
                            if(first16Unit >= 0xD800 && first16Unit <= 0xDBFF) {
                                //high surrogate
                                uint16_t second16Unit;

                                if(decodeIndex + 3 > frameSize) {break;}

                                // find low surrogate
                                if(bigEndian) {
                                    second16Unit = (static_cast<uint8_t>(frameData.data()[decodeIndex + 2]) << 8) | static_cast<uint8_t>(frameData.data()[decodeIndex + 3]); 
                                } else {
                                    second16Unit = static_cast<uint8_t>(frameData.data()[decodeIndex + 2]) | (static_cast<uint8_t>(frameData.data()[decodeIndex + 3]) << 8);
                                }

                                if(second16Unit >= 0xDC00 && second16Unit <= 0xDFFF) {
                                    codepoint = (((first16Unit - 0xD800) << 10) | (second16Unit - 0xDC00)) + 0x10000;
                                    decodeIndex += 4;
                                } else {
                                    break;
                                }
                    
                            } else {
                                // one 16bit character
                                codepoint = first16Unit;

                                decodeIndex += 2;
                            } 

                            // encode codepoint to utf

                            if(codepoint < 128 && (outputPtr->size() < outputPtr->capacity()) ) {
                                // 1 byte
                                outputPtr->push_back(static_cast<char>(codepoint));
                            } else if(codepoint < 2048 && (outputPtr->size() + 1 < outputPtr->capacity()) ) {
                                // 2 bytes
                                outputPtr->push_back(static_cast<char>( 0xC0 | (codepoint >> 6) ));
                                outputPtr->push_back(static_cast<char>( 0x80 | (codepoint & 0x3F) ));
                            } else if(codepoint < 65536 && (outputPtr->size() + 2 < outputPtr->capacity())) {
                                // 3 bytes
                                outputPtr->push_back(static_cast<char>( 0xE0 | (codepoint >> 12) ));
                                outputPtr->push_back(static_cast<char>( 0x80 | ((codepoint >> 6) & 0x3F ) ));
                                outputPtr->push_back(static_cast<char>( 0x80 | (codepoint & 0x3F) ));
                            } else if((outputPtr->size() + 3 < outputPtr->capacity())) {
                                outputPtr->push_back(static_cast<char>( 0xF0 | (codepoint >> 18) ));
                                outputPtr->push_back(static_cast<char>( 0x80 | ((codepoint >> 12) & 0x3F ) ));
                                outputPtr->push_back(static_cast<char>( 0x80 | ((codepoint >> 6) & 0x3F ) ));
                                outputPtr->push_back(static_cast<char>( 0x80 | (codepoint & 0x3F) ));
                            } else {
                                Serial.println("dont fit");
                                break;
                            }
                        }
                    }
                }
            } else if(memcmp(header, "APIC", 4) == 0) {

            }

            file.seek(file.position() + (frameSize - frameBytesRead));

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