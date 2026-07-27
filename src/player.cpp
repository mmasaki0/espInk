#include <vector>
#include <string>
#include <deque>
#include <queue>
#include <atomic>

#include <SD.h>

#include <AudioTools.h>
#include <AudioTools/Communication/A2DPStream.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <BluetoothA2DPSource.h>
#include <esp_avrc_api.h>

#include "player.h"
#include "display.h"
#include "files.h"

std::deque<uint16_t> future; //queue for play next
std::queue<uint16_t> history;

std::atomic_int playing{0};

BufferRTOS<uint8_t> bufferProcessed(1024 * 16);
QueueStream<uint8_t> streamProcessed(bufferProcessed);
ResampleStreamT<ParabolicInterpolator> resampler;
MP3DecoderHelix helix;
EncodedAudioStream decoder(&helix);
VolumeStream volume;
MeasuringStream measure;
// FadeStream fade;
Pipeline pipeline;
// StreamCopy copySongToPipeline(pipeline, currentFile, 1024);
BluetoothA2DPSource a2dp_source;

static TaskHandle_t taskHandleAudio = NULL;
static TaskHandle_t taskHandleAudioControl = NULL;

static QueueHandle_t queueAudioControl;



int32_t a2dpAudioCallback(uint8_t* data, int32_t size) {
    bool p;
    if(streamProcessed.available() == 0) {
        //send empty bytes into a2dp if nothing to play
        memset(data, 0, 512);
        // Serial.println(streamProcessed.available());
        return(512);
    }
    int32_t result = streamProcessed.readBytes((uint8_t*)data, size);
    if(result < size) {
        memset(data + result, 0, size-result);
    }
    // Serial.println(streamProcessed.available());
    // vTaskDelay(pdTICKS_TO_MS(1));
        // Serial.print(result); Serial.print(":"); Serial.print(size); Serial.print(":"); Serial.print(streamProcessed.available());  Serial.println(" ");
    return(size);
}

void avrcCallback(uint8_t id, bool isReleased) {
    if(isReleased) {
        Serial.println(id);
        switch (id) {
            case ESP_AVRC_PT_CMD_PAUSE: {
                
                char qmsg[64] = "PAUSE";
                // Serial.println(qmsg);
                Serial.println(xQueueSend(queueAudioControl, qmsg, 0));
                //temp code to iterate through select position
                // if(xSemaphoreTake(mutexSelect, portMAX_DELAY) == pdTRUE) {
                //     if(playerSelectIndex == 7) {
                //         playerSelectIndex = 0;
                //     } else {
                //         playerSelectIndex++;
                //     }
                //     xSemaphoreGive(mutexSelect);
                //     char dmsg[64] = "PLAYER:";
                //     xQueueSend(queueDisplay, dmsg, 0);
                // }
                //temp code end

                break;
            }   
            case ESP_AVRC_PT_CMD_FORWARD: {
                Serial.println("SEEK:+10");
                char qmsg[64] = "SEEK:+10";
                xQueueSend(queueAudioControl, qmsg, 0);

                //temp code to press currently selected
                // if(xSemaphoreTake(mutexSelect, portMAX_DELAY) == pdTRUE) {
                //     char qmsg[64];
                //     switch (playerSelectIndex) {
                //         case 0: {
                //             // pause
                //             strcpy(qmsg, "PAUSE");
                //             break;
                //         }
                //         case 1: {
                //             // seek +10
                //             strcpy(qmsg, "SEEK:+10");
                //             break;
                //         }
                //         case 2: {
                //             // forward
                //             strcpy(qmsg, "FORWARD");
                //             break;
                //         }
                //         case 3: {
                //             // loop
                //             strcpy(qmsg, "PAUSE");
                //             break;
                //         }
                //         case 4: {
                //             // volume
                //             strcpy(qmsg, "PAUSE");
                //             break;
                //         }
                //         case 5: {
                //             // shuffle
                //             strcpy(qmsg, "PAUSE");
                //             break;
                //         }
                //         case 6: {
                //             // backward
                //             strcpy(qmsg, "BACKWARD");
                //             break;
                //         }
                //         case 7: {
                //             // seek -10
                //             strcpy(qmsg, "SEEK:-10");
                //             break;
                //         }
                //         default: {
                //             break;
                //         }
                //     }
                //     xQueueSend(queueAudioControl, qmsg, 0);
                //     Serial.println(qmsg);
                //     xSemaphoreGive(mutexSelect);
                // }
                //temp code end

                break;
            }
            default:
                break;
        }

    }
}

// takes new path, reopen if mutex is available
void changeCurrentFile(const std::string path) {
    if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
        if(SD.exists(path.c_str())) {
            currentFile.close();

            // decoder.end();
            // decoder.begin();
            // bufferProcessed.reset();

            currentSong = song(path);
            currentSong.loadID3v1();

            currentFile = SD.open(path.c_str());

            Serial.println(streamProcessed.available());
        } else {
            Serial.println("path not found");
        }
        xSemaphoreGive(mutexCurrentFile);

        // displayWriteText(currentSong.id3v1tag.title, 0, 300);
        // displayWriteText(currentSong.id3v1tag.artist, 0, 316);
        // displayWriteText(currentSong.id3v1tag.album, 0, 332);
        // displayWriteText(currentSong.id3v1tag.year, 0, 348);
    }
}

void seekCurrentFile(const uint32_t pos) {
    if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
        currentFile.seek(pos);

        xSemaphoreGive(mutexCurrentFile);
    }
}

uint32_t positionCurrentFile() {
    if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
        uint32_t position = currentFile.position();
        xSemaphoreGive(mutexCurrentFile);
        return position;
    }
    return 0;
}

void taskAudioControl(void *param) {
    while(1) {
        // recieved control message in queue
        char msg[64];
        if(xQueueReceive(queueAudioControl, (void*)&msg, 0) == pdTRUE) {
            Serial.println(msg);
            if(strcmp(msg, "PAUSE") == 0) {
                playing.fetch_xor(1);
                
            }
            else if(strcmp(msg, "FORWARD") == 0) {
                if(!future.empty()) {
                    changeCurrentFile(libraryPaths.at(future.front()));
                    future.pop_front();
                } else {
                    playing = 0;
                    seekCurrentFile(0);
                }
            }
            else if(strcmp(msg, "BACKWARD") == 0) {
                if(!history.empty()) {
                    changeCurrentFile(libraryPaths.at(history.front()));
                    history.pop();
                } else {
                    seekCurrentFile(0);
                }
            }
            else if(strncmp(msg, "SEEK:", 5) == 0) {
                char seekTimeChar[5];
                strncpy(seekTimeChar, &msg[5], 3);
                seekTimeChar[4] = '\0';

                if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
                    uint32_t size = currentFile.size();
                    xSemaphoreGive(mutexCurrentFile);
                    int64_t seekByte = positionCurrentFile() + atoi(seekTimeChar) * measure.bytesPerSecond();

                    if(seekByte < 0) {
                        seekByte = 0;
                    }
                    if(seekByte > size) {
                        seekByte = size;
                    }
                    
                    seekCurrentFile(seekByte);
                }
            }
        }
        //check if file is done reading then skip to next song
        if(playing) {
            if(xSemaphoreTake(mutexCurrentFile, 0) == pdTRUE) {
                int available = currentFile.available();
                xSemaphoreGive(mutexCurrentFile);
                if(available == 0) {
                    char qmsg[64] = "FORWARD";
                    xQueueSend(queueAudioControl, qmsg, 0);
                    Serial.println("done");
                }
                
            }
        }
        vTaskDelay(pdTICKS_TO_MS(10));
    }
    vTaskDelete(NULL);
}

void taskAudio(void *param) {
    uint8_t buffer[1024];

    while(1) {
        if(playing) {
            int bytesRead = 0;

            if(xSemaphoreTake(mutexCurrentFile, 0) == pdTRUE) {
                bytesRead = currentFile.readBytes((char*)buffer, 1024);
                xSemaphoreGive(mutexCurrentFile);
            }

            if(bytesRead > 0) {
                pipeline.write(buffer, bytesRead);    
            }
        } else {
            pipeline.writeSilence(512);
        }


        vTaskDelay(pdTICKS_TO_MS(1));
    }
    vTaskDelete(NULL);
}


void preSetup() {
    queueAudioControl = xQueueCreate(4, 64);
    mutexCurrentFile = xSemaphoreCreateMutex();
    
    xTaskCreatePinnedToCore(taskAudio, "taskAudio", 1024*7, NULL, 15, &taskHandleAudio, 1);
    xTaskCreatePinnedToCore(taskAudioControl, "taskAudioControl", 1024*4, NULL, 10, &taskHandleAudioControl, 1);
}

void setupPipeline() {
    preSetup();

    streamProcessed.begin();

    auto rcfg = resampler.defaultConfig();
    rcfg.sample_rate = 48000;
    rcfg.to_sample_rate = 44100;
    resampler.begin(rcfg);

    // fade.setAudioInfo(AudioInfo(44100, 2, 16));

    pipeline.add(measure);
    pipeline.add(decoder);
    pipeline.add(resampler);
    // pipeline.add(fade);
    // pipeline.add(volume);
    pipeline.setOutput(streamProcessed);
    pipeline.begin();
}

void setupA2DP() {
    // a2dp_source.set_task_core(0);
    a2dp_source.set_volume(60);
    a2dp_source.set_auto_reconnect(true);
    a2dp_source.set_data_callback(a2dpAudioCallback);
    a2dp_source.set_avrc_passthru_command_callback(avrcCallback);
        
    a2dp_source.start("hachiware - Find My");
    
    while(!a2dp_source.is_connected()) {
        Serial.print(".");
        delay(1000);
    }
}

void futureAdd(uint16_t id, std::string loc) {
    if(loc == "front") {
        future.push_front(id);
        if(future.size() > 50) {
            future.pop_back();
        }
    } else if(loc == "back") {
        //only push to back if queue has empty space
        if(future.size() < 50) {
            future.push_back(id);
        }
    }
}
