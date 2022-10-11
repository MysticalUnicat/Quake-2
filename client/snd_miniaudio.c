
// snddma_null.c
// all other sound mixing is portable

#include "client.h"
#include "snd_loc.h"

#include "ext/miniaudio.h"

static struct {
  ma_device device;
  ma_device_config device_config;
  ma_pcm_rb rb;

  ma_uint64 sent;
  ma_uint64 completed_frames;
  ma_uint8 *buffer;
} _;

#define CHANNELS 2
#define FORMAT ma_format_s16
#define FRAME_SIZE ma_get_bytes_per_frame(FORMAT, CHANNELS)
#define CHUNK_COUNT 64
#define CHUNK_MASK 0x3f
#define CHUNK_SIZE 0x400

static void data_callback(ma_device *device, void *output_, const void *input, ma_uint32 frame_count) {
  (void)input;

  ma_uint32 frame_size = FRAME_SIZE;

  ma_uint8 *output = (ma_uint8 *)output_;

  while(frame_count > 0) {
    ma_uint32 size_in_frames = frame_count;
    void *ptr;
    ma_pcm_rb_acquire_read(&_.rb, &size_in_frames, &ptr);
    memcpy(output, ptr, size_in_frames * FRAME_SIZE);
    output += size_in_frames * FRAME_SIZE;
    ma_pcm_rb_commit_read(&_.rb, size_in_frames);
    frame_count -= size_in_frames;
    _.completed_frames += size_in_frames;
  }
}

int SNDDMA_Init(void) {
  ma_uint32 frame_size = FRAME_SIZE;

  memset(&_, 0, sizeof(_));

  _.device_config = ma_device_config_init(ma_device_type_playback);
  _.device_config.playback.channels = CHANNELS;
  _.device_config.playback.format = FORMAT;
  _.device_config.dataCallback = data_callback;

  if(s_khz->value == 44)
    _.device_config.sampleRate = 44100;
  else if(s_khz->value == 22)
    _.device_config.sampleRate = 22050;
  else
    _.device_config.sampleRate = 11025;

  if(ma_device_init(NULL, &_.device_config, &_.device) != MA_SUCCESS) {
    return 0;
  }

  if(ma_pcm_rb_init(FORMAT, CHANNELS, 0x1000, NULL, NULL, &_.rb)) {
    ma_device_uninit(&_.device);
    return 0;
  }

  if(ma_device_start(&_.device) != MA_SUCCESS) {
    ma_device_uninit(&_.device);
    ma_pcm_rb_uninit(&_.rb);
    return 0;
  }

  size_t buffer_size = CHUNK_COUNT * CHUNK_SIZE;
  _.buffer = malloc(buffer_size);

  dma.channels = 2;
  dma.submission_chunk = 512;
  dma.samplepos = 0;
  dma.samplebits = 16;
  dma.samples = buffer_size / (dma.samplebits / 8);
  dma.buffer = _.buffer;
  dma.speed = _.device_config.sampleRate;

  return 1;
}

int SNDDMA_GetDMAPos(void) {
  int s = _.sent * CHUNK_SIZE;
  s >>= 1;
  s &= (dma.samples - 1);
  return s;
}

void SNDDMA_Shutdown(void) {
  ma_device_uninit(&_.device);
  ma_pcm_rb_uninit(&_.rb);
}

void SNDDMA_BeginPainting(void) {}

void SNDDMA_Submit(void) {
  ma_uint32 frame_size = FRAME_SIZE;

  int completed = _.completed_frames / (CHUNK_SIZE / FRAME_SIZE);

  while(((_.sent - completed) >> 1) < 1) {
    if(paintedtime / 256 <= _.sent)
      break;
    ma_uint32 total_size_in_frames = CHUNK_SIZE / FRAME_SIZE;
    while(total_size_in_frames > 0) {
      ma_uint32 size_in_frames = total_size_in_frames;
      void *ptr;
      ma_pcm_rb_acquire_write(&_.rb, &size_in_frames, &ptr);
      memcpy(ptr, _.buffer + ((_.sent & CHUNK_MASK) * CHUNK_SIZE), size_in_frames * FRAME_SIZE);
      ma_pcm_rb_commit_write(&_.rb, size_in_frames);
      total_size_in_frames -= size_in_frames;
    }
    _.sent++;
  }
}

void SNDDMA_DrawStats(void) {
  //   UI_Text("SND sent %llu, completed %llu", _.sent, _.completed_frames / (CHUNK_SIZE / FRAME_SIZE));
}
