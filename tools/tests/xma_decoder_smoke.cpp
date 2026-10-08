#include "audio_probe.h"
#include <cstdarg>
#include <cstdio>

namespace {
void Log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
    std::putchar('\n');
}
}

int main() {
    return TestXmaDecoder(Log) ? 0 : 1;
}
