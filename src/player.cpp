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

uint16_t bufferProcessedSize = 1024 * 16;
BufferRTOS<uint8_t> bufferProcessed(bufferProcessedSize);
QueueStream<uint8_t> streamProcessed(bufferProcessed);

File currentFile;
uint16_t currentId = 0;

std::deque<uint16_t> future; //queue for play next
std::queue<uint16_t> history;

bool playing = true;
bool switching = false;

ResampleStreamT<ParabolicInterpolator> resampler;
MP3DecoderHelix helix;
EncodedAudioStream decoder(&helix);
VolumeStream volume;
Pipeline pipeline;
StreamCopy copySongToPipeline(pipeline, currentFile, 1024 * 8);

BluetoothA2DPSource a2dp_source;

std::map<uint16_t, song> mapLibrary;

static TaskHandle_t taskHandleAudio = NULL;
static TaskHandle_t taskHandleSwitch = NULL;

// static QueueHandle_t playing;
SemaphoreHandle_t mutexPlaying;

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

void setPlaying(bool nextState) {
    if(xSemaphoreTake(mutexPlaying, portMAX_DELAY) == pdTRUE) {
        playing = nextState;
        xSemaphoreGive(mutexPlaying);
    }
}

void taskAudio(void *param) {
    char msg[256];
    while(1) {
        // if(xSemaphoreTake(mutexAudio, 0) == pdTRUE) {
            // if(!playing) {
            // static uint8_t zeros[512] = {0};
            // streamProcessed.write(zeros, sizeof(zeros));
            // } else {
            if(playing) {
                int bytesRead = copySongToPipeline.copy();
                if(currentFile.available() == 0) {
                    playing = false;
                    Serial.println("done");
                }
            }

            
            // Serial.println(bytesRead);

            // }
            // xSemaphoreGive(mutexAudio);
        // } else {
        //     vTaskDelay(pdTICKS_TO_MS(1));
        // }
        vTaskDelay(pdTICKS_TO_MS(1));
    }
    vTaskDelete(NULL);
}

void avrcCallback(uint8_t id, bool isReleased) {
    if(isReleased) {

        switch (id) {
            case ESP_AVRC_PT_CMD_PAUSE:
                Serial.println("pause");
                playing = !playing;
                break;
            case ESP_AVRC_PT_CMD_FORWARD: {
                Serial.println("skip forward");
                // playing=false;
                  
                currentFile = SD.open(mapLibrary[future.front()].path);
                  
                // Serial.println("buh");
                future.pop_front();
                // playing=true;
                // streamProcessed.clear();
                // Serial.println(future.front());
                // xQueueSend(queueHandleSwitch, (void *)mapLibrary[future.front()].path, 10);
                // Serial.println(currentFile.name());
                // switching = true;
                break;
            }
            default:
                break;
        }

    }
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
    // playing = xQueueCreate(8, sizeof(bool));
    xTaskCreatePinnedToCore(taskAudio, "taskAudio", 1024*3, NULL, 15, &taskHandleAudio, 1);
    mutexPlaying = xSemaphoreCreateMutex();
    // xTaskCreatePinnedToCore(taskSwitchCurrentFile, "taskSwitchCurrentFile", 1024*3, NULL, 20, &taskHandleSwitch, 1);

    

    currentFile = SD.open("/library/ARIRANG/SWIM.mp3");
}

void setupPipeline() {
    preSetup();

    streamProcessed.begin();

    auto rcfg = resampler.defaultConfig();
    rcfg.sample_rate = 48000;
    rcfg.to_sample_rate = 44100;
    resampler.begin(rcfg);

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

void futureNext() {
    if(future.front()) {

    } else {
        playing = false;
    }
}

