#include <map>
#include "player.h"

#include <SD.h>

#include <AudioTools.h>
#include <AudioTools/Communication/A2DPStream.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Disk/AudioSourceSD.h>

#include <BluetoothA2DPSource.h>
#include <esp_avrc_api.h>

#include <deque>
#include <queue>
#include <atomic>

uint16_t bufferProcessedSize = 1024 * 16;
BufferRTOS<uint8_t> bufferProcessed(bufferProcessedSize);
QueueStream<uint8_t> streamProcessed(bufferProcessed);

File currentFile;
uint16_t currentId = 0;

std::deque<uint16_t> future; //queue for play next
std::queue<uint16_t> history;

std::atomic_int playing{0};
bool switching = false;

ResampleStreamT<ParabolicInterpolator> resampler;
MP3DecoderHelix helix;
EncodedAudioStream decoder(&helix);
VolumeStream volume;
MeasuringStream measure;
Pipeline pipeline;
StreamCopy copySongToPipeline(pipeline, currentFile, 1024 * 8);
BluetoothA2DPSource a2dp_source;

std::map<uint16_t, song> mapLibrary;

static TaskHandle_t taskHandleAudio = NULL;
static TaskHandle_t taskHandleAudioControl = NULL;

static QueueHandle_t queueAudioControl;

SemaphoreHandle_t mutexCurrentFile;

int32_t a2dpAudioCallback(uint8_t* data, int32_t size) {
    bool p;
    if(!playing || streamProcessed.available() == 0) {
        //send empty bytes into a2dp if paused
        memset(data, 0, 512);
        // Serial.println(streamProcessed.available());
        return(512);
    }
    int32_t result = streamProcessed.readBytes((uint8_t*)data, size);
    if(result < size) {
        memset(data + result, 0, size-result);
    }
    // Serial.println(streamProcessed.available());
    vTaskDelay(pdTICKS_TO_MS(1));
        // Serial.print(result); Serial.print(":"); Serial.print(size); Serial.print(":"); Serial.print(streamProcessed.available());  Serial.println(" ");
    return(size);
}

void avrcCallback(uint8_t id, bool isReleased) {
    if(isReleased) {
        Serial.println(id);
        switch (id) {
            case ESP_AVRC_PT_CMD_PAUSE: {
                Serial.println("PAUSE");
                char qmsg[64] = "PAUSE";
                xQueueSend(queueAudioControl, qmsg, 0);
                break;
            }   
            case ESP_AVRC_PT_CMD_FORWARD: {
                Serial.println("FORWARD");
                // playing=false;
                char qmsg[64] = "FORWARD";
                xQueueSend(queueAudioControl, qmsg, 0);
                break;
            }
            default:
                break;
        }

    }
}

// takes new path, reopen if mutex is available
void changeCurrentFile(const char* path) {
    if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
        if(SD.exists(path)) {
            currentFile.close();
            currentFile = SD.open(path);
        } else {
            Serial.println("path not found");
        }
        xSemaphoreGive(mutexCurrentFile);
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

uint32_t sizeCurrentFile() {
    if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
        uint32_t size = currentFile.size();
        xSemaphoreGive(mutexCurrentFile);
        return size;
    }
    return 0;
}

int availableCurrentFile() {
    if(xSemaphoreTake(mutexCurrentFile, portMAX_DELAY) == pdTRUE) {
        int available = currentFile.available();
        xSemaphoreGive(mutexCurrentFile);
        return available;
    }
    return 0;
}

void taskAudioControl(void *param) {
    while(1) {
        // recieved control message in queue
        char msg[64];
        if(xQueueReceive(queueAudioControl, (void*)&msg, 0) == pdTRUE) {
            if(strcmp(msg, "PAUSE") == 0) {
                playing.fetch_xor(1);
            }
            if(strcmp(msg, "FORWARD") == 0) {
                if(!future.empty()) {
                    changeCurrentFile(mapLibrary[future.front()].path);
                    future.pop_front();
                } else {
                    playing = 0;
                }
            }
            if(strcmp(msg, "BACKWARD") == 0) {
                if(!history.empty()) {
                    changeCurrentFile(mapLibrary[history.front()].path);
                    history.pop();
                } else {
                    seekCurrentFile(0);
                }
            }
            if(strncmp(msg, "SEEK:", 5) == 0) {
                char seekTimeChar[5];
                strncpy(seekTimeChar, &msg[5], 3);
                seekTimeChar[4] = '\0';
                uint32_t size = sizeCurrentFile();
                int64_t seekByte = positionCurrentFile() + atoi(seekTimeChar) * measure.bytesPerSecond();
                if(seekByte < 0) {
                    seekByte = 0;
                }
                if(seekByte > size) {
                    seekByte = size;
                }
                Serial.println(seekByte);
                seekCurrentFile(seekByte);
            }
        }
        if(playing) {
            if(availableCurrentFile() == 0) {
                char qmsg[64] = "FORWARD";
                xQueueSend(queueAudioControl, qmsg, 0);
                Serial.println("done");
            }
        }
        vTaskDelay(pdTICKS_TO_MS(1));
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
        vTaskDelay(pdTICKS_TO_MS(1));
    }
    vTaskDelete(NULL);
}


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

void preSetup() {
    queueAudioControl = xQueueCreate(4, 64);
    mutexCurrentFile = xSemaphoreCreateMutex();
    
    xTaskCreatePinnedToCore(taskAudio, "taskAudio", 1024*3, NULL, 15, &taskHandleAudio, 1);
    xTaskCreatePinnedToCore(taskAudioControl, "taskAudioControl", 1024*3, NULL, 10, &taskHandleAudioControl, 1);

    
    changeCurrentFile("/library/ARIRANG/SWIM.mp3");
}

void setupPipeline() {
    preSetup();

    streamProcessed.begin();

    auto rcfg = resampler.defaultConfig();
    rcfg.sample_rate = 48000;
    rcfg.to_sample_rate = 44100;
    resampler.begin(rcfg);

    pipeline.add(measure);
    pipeline.add(decoder);
    pipeline.add(resampler);
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
    Serial.print("buffer size:"); Serial.println(DEFAULT_BUFFER_SIZE); Serial.println(A2DP_BUFFER_SIZE);
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
