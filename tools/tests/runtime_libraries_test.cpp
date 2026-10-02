#include "os/runtime_libraries.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using os::runtime_libraries::Find;
using os::runtime_libraries::Holds;

static void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

static void Touch(const fs::path& path)
{
    fs::create_directories(path.parent_path());
    std::ofstream(path) << "library";
}

int main()
{
    const fs::path root = fs::temp_directory_path() / "lo-runtime-libraries-test";
    std::error_code error;
    fs::remove_all(root, error);
    const fs::path executable = root / "bin";
    const fs::path working = root / "work";
    fs::create_directories(executable);
    fs::create_directories(working);

    Check(Find("nvngx_dlss.dll", executable, working) == working, "missing library did not fall back to the working directory");
    Check(Find("nvngx_dlss.dll", {}, working) == working, "unknown executable directory did not fall back");

    // Windows packages and source builds stage the runtime beside the executable.
    Touch(executable / "nvngx_dlss.dll");
    Check(Find("nvngx_dlss.dll", executable, working) == executable, "Windows runtime beside the executable not found");

    // Linux source builds stage the versioned snippet beside the executable.
    Check(!Holds(executable, "libnvidia-ngx-dlss.so"), "absent Linux snippet reported");
    Touch(executable / "libnvidia-ngx-dlss.so.310.9.1");
    Check(Find("libnvidia-ngx-dlss.so", executable, working) == executable, "versioned Linux snippet not found");
    Check(!Holds(executable, "libnvidia-ngx-dlssd.so"), "another NGX feature matched the DLSS snippet");

    // Linux packages keep the unmodified snippet in ngx/, which comes first.
    Touch(executable / "ngx" / "libnvidia-ngx-dlss.so.310.9.1");
    Check(Find("libnvidia-ngx-dlss.so", executable, working) == executable / "ngx", "packaged ngx/ folder not preferred");

    // A directory named like the library is not the library.
    fs::create_directories(root / "dir" / "nvngx_dlss.dll");
    fs::create_directories(root / "dir" / "libnvidia-ngx-dlss.so.1");
    Check(!Holds(root / "dir", "nvngx_dlss.dll"), "directory accepted as a library");
    Check(!Holds(root / "dir", "libnvidia-ngx-dlss.so"), "directory accepted as a versioned library");

    // Names outside the active code page must not stop the search on Windows;
    // Hangul is outside both Western and Chinese ANSI code pages.
    Touch(root / "unicode" / fs::path(u8"게임ゲーム.txt"));
    Touch(root / "unicode" / "libnvidia-ngx-dlss.so.310.9.1");
    Check(Holds(root / "unicode", "libnvidia-ngx-dlss.so"), "non-ASCII file name broke the search");
    // A miss visits every name, including the non-ASCII one.
    Check(!Holds(root / "unicode", "libnvidia-ngx-dlssd.so"), "non-ASCII file name matched");

    fs::remove_all(root, error);
    return 0;
}
