#include "optiscaler_loader.h"
#include <cstdlib>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace gpu::optiscaler {
namespace {
LoadResult Load(bool ngxCompiled) {
    LoadResult result;
#ifdef _WIN32
    // Read the Windows environment directly so paths outside the active ANSI
    // code page work too. A bare filename never enters DLL search resolution.
    const DWORD length = GetEnvironmentVariableW(L"LO_OPTISCALER_PATH", nullptr, 0);
    if (!length) return result;
    result.requested = true;
    std::wstring configured(length, L'\0');
    const DWORD copied = GetEnvironmentVariableW(L"LO_OPTISCALER_PATH", configured.data(), length);
    if (!copied || copied >= length) {
        result.systemError = GetLastError();
        result.reason = "LO_OPTISCALER_PATH changed or could not be read";
        return result;
    }
    configured.resize(copied);
    result.path = std::filesystem::path(configured);
    if (!ngxCompiled) {
        result.reason = "OptiScaler integration requires a build with the DLSS/NGX input adapter";
        return result;
    }
    const char* provider = std::getenv("LO_FG_PROVIDER");
    if (!provider || std::string_view(provider) != "off") {
        result.reason = "set LO_FG_PROVIDER=off before loading OptiScaler to keep one FG presentation owner";
        return result;
    }
    if (!result.path.is_absolute()) {
        result.reason = "LO_OPTISCALER_PATH must be an absolute path to OptiScaler.dll";
        return result;
    }
    const auto name = result.path.filename().wstring();
    if (CompareStringOrdinal(name.c_str(), -1, L"OptiScaler.dll", -1, TRUE) != CSTR_EQUAL) {
        result.reason = "explicit loading requires the original OptiScaler.dll filename";
        return result;
    }
    std::error_code error;
    if (!std::filesystem::is_regular_file(result.path, error)) {
        result.systemError = uint32_t(error.value());
        result.reason = "OptiScaler.dll does not exist or is not a regular file";
        return result;
    }
    const auto canonical = std::filesystem::canonical(result.path, error);
    if (error) {
        result.systemError = uint32_t(error.value());
        result.reason = "OptiScaler.dll path could not be resolved";
        return result;
    }
    result.path = canonical;
    // OptiScaler attaches its API hooks in DllMain. Do not unload it after a
    // backend reset or on a later error: live dispatch pointers may target it.
    static HMODULE processModule = nullptr;
    // A file that is not a valid DLL would make Windows show a modal "Bad Image"
    // dialog and block startup. Fail the load instead, for this thread only.
    DWORD previousMode = 0;
    const BOOL quiet = SetThreadErrorMode(GetThreadErrorMode() | SEM_FAILCRITICALERRORS, &previousMode);
    processModule = LoadLibraryExW(result.path.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    const DWORD loadError = GetLastError();
    if (quiet) SetThreadErrorMode(previousMode, nullptr);
    if (!processModule) {
        result.systemError = loadError;
        result.reason = "OptiScaler DLL loading failed";
        return result;
    }
    result.loaded = true;
    result.reason = "DLL loaded; NGX input and the selected OptiScaler output still require runtime validation";
#else
    (void)ngxCompiled;
    const char* configured = std::getenv("LO_OPTISCALER_PATH");
    if (configured && *configured) {
        result.requested = true;
        result.reason = "OptiScaler is a Windows DLL; this native platform cannot load it";
    }
#endif
    return result;
}
} // namespace

const LoadResult& Initialize(bool ngxCompiled) {
    static const LoadResult result = Load(ngxCompiled);
    return result;
}
} // namespace gpu::optiscaler
