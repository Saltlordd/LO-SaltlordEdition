#include "audio_probe.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}
#include <initializer_list>

bool TestXmaDecoder(void (*log)(const char*, ...)) {
    log("FFmpeg libavcodec=%u libavutil=%u license=%s", avcodec_version(), avutil_version(), avcodec_license());
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_XMAFRAMES);
    if (!codec) { log("BLOCKED: XMAFRAMES decoder is missing"); return false; }
    bool ready = true;
    for (int channels : {1, 2}) {
        for (int rate : {24000, 32000, 44100, 48000}) {
            AVCodecContext* context = avcodec_alloc_context3(codec);
            AVPacket* packet = av_packet_alloc();
            AVFrame* frame = av_frame_alloc();
            int opened = AVERROR(ENOMEM);
            int rejected = AVERROR(ENOMEM);
            if (context && packet && frame) {
                // Match the production apu/xma.cpp decoder setup and API.
                context->channels = channels;
                context->sample_rate = rate;
                context->thread_count = 1;
                opened = avcodec_open2(context, codec, nullptr);
                if (opened >= 0 && av_new_packet(packet, 2) >= 0) {
                    packet->data[0] = packet->data[1] = 0;
                    rejected = avcodec_send_packet(context, packet);
                    if (rejected >= 0) rejected = avcodec_receive_frame(context, frame);
                    avcodec_flush_buffers(context);
                }
            }
            const bool passed = opened >= 0 && rejected == AVERROR_INVALIDDATA;
            ready &= passed;
            log("XMA decoder smoke: channels=%d rate=%d open=%d malformed_packet=%d expected=%d result=%s",
                channels, rate, opened, rejected, AVERROR_INVALIDDATA, passed ? "PASS" : "FAIL");
            av_frame_free(&frame);
            av_packet_free(&packet);
            avcodec_free_context(&context);
        }
    }
    log("XMA game-frame decoding NOT EXERCISED: no encoded game-audio sample in this APK");
    return ready;
}
