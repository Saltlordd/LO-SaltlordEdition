#include "gpu/optiscaler_loader.h"
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
void Check(bool okay, const char* why) {
    if (!okay) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    Check(argc == 3, "scenario and fixture path required");
    const std::wstring_view scenario = argv[1];
    const auto length = GetFullPathNameW(argv[2], 0, nullptr, nullptr);
    Check(length != 0, "absolute fixture path length");
    std::wstring full(length, L'\0');
    const auto copied = GetFullPathNameW(argv[2], length, full.data(), nullptr);
    Check(copied > 0 && copied < length, "absolute fixture path");
    full.resize(copied);
    const std::filesystem::path fixture(full);
    const auto cwd = std::filesystem::current_path();
    const auto errorMode = GetThreadErrorMode();
    Check(_putenv_s("LO_FG_PROVIDER", "off") == 0, "set internal FG owner");
    SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", full.c_str());
    bool ngxCompiled = true;
    if (scenario == L"disabled") SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", nullptr);
    else if (scenario == L"no-sdk") ngxCompiled = false;
    else if (scenario == L"conflict") Check(_putenv_s("LO_FG_PROVIDER", "fsr") == 0, "select internal FG");
    else if (scenario == L"relative") SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", L"OptiScaler.dll");
    else if (scenario == L"wrong-name") {
        const auto wrong = fixture.parent_path() / "another.dll";
        SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", wrong.c_str());
    } else if (scenario == L"missing") {
        const auto missing = fixture.parent_path() / "absent-loader-fixture" / "OptiScaler.dll";
        Check(!std::filesystem::exists(missing), "missing fixture really absent");
        SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", missing.c_str());
    } else if (scenario == L"invalid-dll") {
        const auto invalid = fixture.parent_path() / "invalid-loader-fixture" / "OptiScaler.dll";
        std::filesystem::create_directories(invalid.parent_path());
        std::ofstream(invalid, std::ios::binary) << "This is not a PE DLL.";
        SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", invalid.c_str());
    } else if (scenario == L"loaded") {
        const auto unicode = fixture.parent_path() / L"\u63d2\u5e27 space" / "OptiScaler.dll";
        std::filesystem::create_directories(unicode.parent_path());
        Check(CopyFileW(fixture.c_str(), unicode.c_str(), FALSE), "copy Unicode-path fixture");
        SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", unicode.c_str());
    }

    const auto& result = gpu::optiscaler::Initialize(ngxCompiled);
    Check(result.requested == (scenario != L"disabled"), "explicit opt-in only");
    Check(result.loaded == (scenario == L"loaded"), "only admissible DLL was attached");
    Check(std::filesystem::current_path() == cwd, "loader preserves caller working directory");
    Check(GetThreadErrorMode() == errorMode, "loader restores caller thread error mode");
    if (scenario == L"loaded") {
        Check(result.path.parent_path().filename() == L"\u63d2\u5e27 space", "Unicode path preserved");
        const auto module = GetModuleHandleW(result.path.c_str());
        Check(module != nullptr, "module remains resident after initialization");
        const auto probe = std::bit_cast<unsigned(*)()>(GetProcAddress(module, "LoOptiscalerLoaderFixtureAttachments"));
        Check(probe && probe() == 1, "actual DLL attachment ran once");
        SetEnvironmentVariableW(L"LO_OPTISCALER_PATH", L"missing.dll");
        const auto& again = gpu::optiscaler::Initialize(false);
        Check(&again == &result && again.loaded && GetModuleHandleW(result.path.c_str()) == module,
            "GPU reinitialization retains the original hook module and status");
    } else {
        Check(GetModuleHandleW(L"OptiScaler.dll") == nullptr, "rejection never attaches the DLL");
        if (scenario != L"disabled") Check(!result.reason.empty(), "failure gives a reason");
        if (scenario == L"invalid-dll") Check(result.systemError != 0, "native load error retained");
    }
    std::puts("PASS OptiScaler loader lifecycle (fixture DLL, no SDK or GPU)");
    return 0;
}
#else
int main(int argc, char** argv) {
    Check(argc == 2, "scenario required");
    const bool requested = std::string_view(argv[1]) != "disabled";
    if (requested) setenv("LO_OPTISCALER_PATH", "/tmp/OptiScaler.dll", 1);
    else unsetenv("LO_OPTISCALER_PATH");
    const auto& result = gpu::optiscaler::Initialize(true);
    Check(result.requested == requested && !result.loaded, "native non-Windows build cannot advertise loaded DLL");
    if (requested) Check(!result.reason.empty(), "unsupported platform has a reason");
    std::puts("PASS OptiScaler platform boundary");
}
#endif
