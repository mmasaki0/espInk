#include "player.h"

#include <AudioTools.h>
#include <AudioTools/Communication/A2DPStream.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Disk/AudioSourceSD.h>

#include <BluetoothA2DPSource.h>

uint16_t bufferProcessedSize = 1024 * 8;
BufferRTOS<uint8_t> bufferProcessed(bufferProcessedSize);
QueueStream<uint8_t> streamProcessed(bufferProcessed);

File song1;

ResampleStreamT<ParabolicInterpolator> resampler;
MP3DecoderHelix helix;
EncodedAudioStream decoder(&helix);
Pipeline pipeline;
StreamCopy copySongToPipeline(pipeline, song1);

BluetoothA2DPSource a2dp_source;

int32_t a2dpAudioCallback(uint8_t* data, int32_t size) {
  int32_t result = streamProcessed.readBytes((uint8_t*)data, size);
  if(result < size) {
    memset(data + result, 0, size-result);
  }
  delay(1);
  return(size);
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
    a2dp_source.set_data_callback(a2dpAudioCallback);
    a2dp_source.set_auto_reconnect(true);
    a2dp_source.start("hachiware - Find My");

    while(!a2dp_source.is_connected()) {
        Serial.print(".");
        delay(1000);
    }
}

