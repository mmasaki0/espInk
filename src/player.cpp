#include <map>
#include "player.h"

#include <AudioTools.h>
#include <AudioTools/Communication/A2DPStream.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Disk/AudioSourceSD.h>

#include <BluetoothA2DPSource.h>

uint16_t bufferProcessedSize = 1024 * 4;
BufferRTOS<uint8_t> bufferProcessed(bufferProcessedSize);
QueueStream<uint8_t> streamProcessed(bufferProcessed);

File song1;
uint64_t currentId = 0;

ResampleStreamT<ParabolicInterpolator> resampler;
MP3DecoderHelix helix;
EncodedAudioStream decoder(&helix);
Pipeline pipeline;
StreamCopy copySongToPipeline(pipeline, song1);

BluetoothA2DPSource a2dp_source;

std::map<uint64_t, song> mapLibrary;

int32_t a2dpAudioCallback(uint8_t* data, int32_t size) {
  int32_t result = streamProcessed.readBytes((uint8_t*)data, size);
  if(result < size) {
    memset(data + result, 0, size-result);
  }
  // delay(1);
  // bob = streamProcessed.levelPercent();
//   Serial.print(result); Serial.print(":"); Serial.print(size); Serial.print(" "); Serial.print(streamProcessed.available());  Serial.print(" ");
  return(size);
}


void sdTraverse(const char* path) {
    Serial.println("bro");
    File dir = SD.open(path);
    File next = dir.openNextFile();
    // while(next) {
    //     // delay(1);
    //     // Serial.println(next.path());
    //     if(next.isDirectory()) {
    //         char newpath[256];
    //         snprintf(newpath, sizeof(newpath), "%s/%s", path, next.name());
    //         sdTraverse(newpath);
    //     } else {
    //         char extension[4];
    //         strncpy(extension, next.name() + strlen(next.name()) - 3, 3);
    //         extension[3] = '\0';
    //         // Serial.println(extension);
    //         if(strcmp(extension, "mp3") == 0) {
    //             Serial.println(next.name());
    //             mapLibrary[currentId++] = song(next.path());
    //         }
    //     }
    //     next.close();
    //     next = dir.openNextFile();
    // }
    next.close();
    dir.close();
}

void setupPipeline() {
    song1 = SD.open("/library/ARIRANG/2.0.mp3");

    streamProcessed.begin();

    auto rcfg = resampler.defaultConfig();
    rcfg.sample_rate = 48000;
    rcfg.to_sample_rate = 44100;
    resampler.begin(rcfg);

    pipeline.add(decoder);
    pipeline.add(resampler);
    pipeline.setOutput(streamProcessed);
    pipeline.begin();
}

void setupA2DP() {
    a2dp_source.set_volume(60);
    a2dp_source.set_auto_reconnect(true);
    a2dp_source.start("hachiware - Find My");
    a2dp_source.set_data_callback(a2dpAudioCallback);
    while(!a2dp_source.is_connected()) {
        Serial.print(".");
        delay(1000);
    }
    
}

