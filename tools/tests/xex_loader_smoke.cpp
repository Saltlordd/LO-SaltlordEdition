#include "xex_probe.h"
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <chrono>

namespace {
void Log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
    std::putchar('\n');
}
}

int main(int argc, char** argv) {
    if (argc != 2) { std::puts("Usage: LoXexLoaderSmoke <Disc 1 default.xex>"); return 2; }
    if (TestXexImage(argv[1], Log) != XexProbeResult::Loaded) return 1;
    const auto invalid = std::filesystem::temp_directory_path() / ("lo-xex-smoke-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tmp");
    std::ofstream(invalid, std::ios::binary).put('x');
    const auto rejected = TestXexImage(invalid, Log);
    std::filesystem::remove(invalid);
    const auto missing = TestXexImage(invalid, Log);
    std::filesystem::copy_file(argv[1], invalid);
    { std::fstream changed(invalid, std::ios::binary | std::ios::in | std::ios::out); changed.put('x'); }
    const auto badHash = TestXexImage(invalid, Log);
    std::filesystem::remove(invalid);
    return rejected == XexProbeResult::Rejected && missing == XexProbeResult::Missing &&
        badHash == XexProbeResult::Rejected ? 0 : 1;
}
