#pragma once

// XAudio render driver emulation. The game registers a callback that the
// driver invokes whenever it wants another frame of 256 samples x 6 channels
// (48 kHz, 32-bit float, planar); the game answers with
// XAudioSubmitRenderDriverFrame. Convert big-endian planar floats to stereo
// PCM and queue them to SDL; bounded device buffering paces the callback.

#define XAUDIO_SAMPLES_HZ 48000
#define XAUDIO_NUM_CHANNELS 6
#define XAUDIO_NUM_SAMPLES 256

namespace apu
{
    void Init();
    void RegisterClient(uint32_t callback, uint32_t param);
    // Disables future calls and drains a call already acquired by the driver.
    // When called from that callback itself, it disables subsequent calls.
    void UnregisterClient();
    void SubmitFrame(const void* samples);
    void SetPaused(bool paused);
}
