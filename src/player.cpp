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

Pipeline pipeline;
const int bufferProcessedSize = 1024 * 16;
BufferRTOS<uint8_t> bufferProcessed(bufferProcessedSize);
QueueStream<uint8_t> streamProcessed(bufferProcessed);
MeasuringStream measure;
MP3DecoderHelix helix;
EncodedAudioStream decoder(&helix);
ResampleStreamT<LinearInterpolator> resampler;
FadeStream fade;
VolumeStream volume;
StreamCopy copySongToPipeline(pipeline, currentFile, 1024);
BluetoothA2DPSource a2dp_source;

static TaskHandle_t taskHandleAudio = NULL;
static TaskHandle_t taskHandleAudioControl = NULL;

static QueueHandle_t queueAudioControl;



int32_t a2dpAudioCallback(uint8_t* data, int32_t size) {
    if(streamProcessed.available() == 0) {
        // return nothing to prevent blocking
        memset(data, 0, size);
        return size;
    }

    int32_t result = streamProcessed.readBytes((uint8_t*)data, size);
    if(result < size) {
        memset(data + result, 0, size-result);
    }

    // do not put delay here
    return(size);
}

void avrcCallback(uint8_t id, bool isReleased) {
    if(isReleased) {
        // Serial.println(id);
        switch (id) {
            case ESP_AVRC_PT_CMD_PAUSE: {
                
                char qmsg[64] = "PAUSE";
                // Serial.println(qmsg);
                xQueueSend(queueAudioControl, qmsg, 0);
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
                // Serial.println("FORWARD");
                char qmsg[64] = "FORWARD";
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

            // NEED TO SET VOLUME TO 0 then back to 1 after because garbage leaks through 
            volume.setVolume(0);
            pipeline.end();
            bufferProcessed.reset();
            currentSong = song(path);
            
            // Serial.println(currentSong.id3v2.exists); Serial.println(currentSong.id3v2.size);
            
            setupPipeline();
            currentFile = SD.open(path.c_str());
            xSemaphoreGive(mutexCurrentFile);
            volume.setVolume(1);
        } else {
            Serial.println("path not found");
        }
        
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
    TickType_t lastBitrateTick = 0;
    int meanBitrate = 0;
    while(1) {
        // recieved control message in queue
        char msg[64];
        if(xQueueReceive(queueAudioControl, (void*)&msg, 0) == pdTRUE) {
            if(strcmp(msg, "PAUSE") == 0) {
                if(playing) {
                    fade.setFadeOutActive(true);
                    // vTaskDelay(1000);
                } else {
                    fade.setFadeInActive(true);
                }
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
                    // int64_t seekByte = positionCurrentFile() + atoi(seekTimeChar) * measure.bytesPerSecond();
                    int64_t seekByte = positionCurrentFile() + atoi(seekTimeChar) * meanBitrate;

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
        
        if(playing) {
            // check if file is done reading then skip to next song
            if(xSemaphoreTake(mutexCurrentFile, 0) == pdTRUE) {
                int available = currentFile.available();
                xSemaphoreGive(mutexCurrentFile);
                if(available == 0) {
                    char qmsg[64] = "FORWARD";
                    xQueueSend(queueAudioControl, qmsg, 0);
                    Serial.println("done");
                }
                
            }
            
            // calculate mean bitrate used for seeking
            // if((xTaskGetTickCount() - lastBitrateTick) > 500) {
            //     lastBitrateTick = xTaskGetTickCount();
            //     meanBitrate = (0.5 * meanBitrate) + (0.5 * measure.bytesPerSecond());
            //     // Serial.print("m:"); Serial.println(meanBitrate);

            //     Serial.println(decoder.available());
            // }
            
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void taskAudio(void *param) {
    while(1) {
        if(playing) {
            if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
                copySongToPipeline.copy();
                xSemaphoreGive(mutexCurrentFile);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
    vTaskDelete(NULL);
}


void preSetup() {
    queueAudioControl = xQueueCreate(4, 64);
    mutexCurrentFile = xSemaphoreCreateMutex();
    
    xTaskCreatePinnedToCore(taskAudio, "taskAudio", 1024*8, NULL, 15, &taskHandleAudio, 1);
    xTaskCreatePinnedToCore(taskAudioControl, "taskAudioControl", 1024*5, NULL, 10, &taskHandleAudioControl, 1);
}

void setupPipeline() {
    streamProcessed.begin();

    auto rcfg = resampler.defaultConfig();
    rcfg.to_sample_rate = 44100;
    resampler.begin(rcfg);
    fade.setAudioInfo(rcfg);

    pipeline.add(measure);
    pipeline.add(decoder);
    pipeline.add(resampler);
    pipeline.add(fade);
    pipeline.add(volume);
    
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
