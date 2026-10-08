#include "audio_probe.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

bool CheckPcm(void (*log)(const char*, ...)) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        log("BLOCKED SDL audio initialization: %s", SDL_GetError()); return false;
    }
    log("SDL audio driver=%s; output devices=%d", SDL_GetCurrentAudioDriver(), SDL_GetNumAudioDevices(0));
    SDL_AudioSpec wanted{}, actual{};
    wanted.freq = 48000;
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 2;
    wanted.samples = 512;
    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &actual,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (!device) {
        log("BLOCKED SDL audio output: %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO); return false;
    }
    log("SDL PCM output opened: rate=%d channels=%u format=0x%04X samples=%u",
        actual.freq, actual.channels, actual.format, actual.samples);
    bool passed = actual.freq > 0 && actual.format == AUDIO_F32SYS && actual.channels == 2;
    if (passed) {
        // Quiet, faded stereo tone; no copyrighted asset and no microphone access.
        const int frames = actual.freq * 3 / 4;
        const int fade = std::max(1, actual.freq / 50);
        std::vector<float> pcm(frames * 2);
        constexpr double tau = 6.283185307179586;
        for (int i = 0; i < frames; ++i) {
            const float envelope = std::min(1.0f, float(std::min(i, frames - 1 - i)) / fade);
            pcm[i * 2] = float(std::sin(tau * 440 * i / actual.freq)) * 0.08f * envelope;
            pcm[i * 2 + 1] = float(std::sin(tau * 660 * i / actual.freq)) * 0.08f * envelope;
        }
        passed = SDL_QueueAudio(device, pcm.data(), Uint32(pcm.size() * sizeof(float))) == 0;
        if (passed) {
            log("PCM tone queued: 750 ms; left=440 Hz right=660 Hz; audible output requires user confirmation");
            SDL_PauseAudioDevice(device, 0);
            const Uint32 start = SDL_GetTicks();
            while (SDL_GetQueuedAudioSize(device) && SDL_GetTicks() - start < 3000) SDL_Delay(20);
            const Uint32 remaining = SDL_GetQueuedAudioSize(device);
            passed = remaining == 0;
            log("SDL PCM queue consumption=%s remaining_bytes=%u elapsed_ms=%u; speaker audibility NOT VERIFIED",
                passed ? "PASS" : "FAIL", remaining, SDL_GetTicks() - start);
        } else log("BLOCKED SDL_QueueAudio: %s", SDL_GetError());
    } else log("BLOCKED unexpected PCM format; no tone submitted");
    SDL_ClearQueuedAudio(device);
    SDL_CloseAudioDevice(device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return passed;
}
}

AudioProbeResult RunAudioProbe(void (*log)(const char*, ...)) {
    AudioProbeResult result;
    result.decoderReady = TestXmaDecoder(log);
    result.pcmReady = CheckPcm(log);
    return result;
}
