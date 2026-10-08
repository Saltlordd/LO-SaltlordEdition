#pragma once

struct AudioProbeResult {
    bool decoderReady = false;
    bool pcmReady = false;
};

// The decoder check does not consume or claim to decode game audio.
AudioProbeResult RunAudioProbe(void (*log)(const char*, ...));
bool TestXmaDecoder(void (*log)(const char*, ...));
