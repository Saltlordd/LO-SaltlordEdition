#include "os/user_paths.h"
#include "updater/update.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void WriteSettings(const std::filesystem::path& path, const char* contents)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    output << contents;
    Check(bool(output), "write isolated settings fixture");
}
}

int main()
{
    namespace fs = std::filesystem;
    const auto previousDirectory = fs::current_path();
    const auto root = fs::temp_directory_path() /
        ("lo-settings-path-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
#ifndef _WIN32
    const char* originalXdg = std::getenv("XDG_CONFIG_HOME");
    const bool hadXdg = originalXdg != nullptr;
    const std::string previousXdg = hadXdg ? originalXdg : "";
    const char* actualHome = std::getenv("HOME");
    const auto fallback = (actualHome ? fs::path(actualHome) : fs::path{}) /
        ".config/lost-odyssey-recomp/settings.ini";
#endif
    int result = 0;
    try
    {
        fs::create_directories(root / "portable");
        fs::create_directories(root / "launch");
        fs::current_path(root / "launch");
        os::user_paths::Initialize(root / "portable");
        Check(os::user_paths::SettingsPath() == "settings.ini", "portable settings must retain working-directory contract");
        Check(!fs::exists(os::user_paths::SettingsPath()), "missing portable settings trigger initial setup");
        WriteSettings(root / "launch/settings.ini", "automatic_updates=0\nui_language=2\n");
        Check(fs::exists(os::user_paths::SettingsPath()), "saved portable settings bypass initial setup");
        auto preferences = updater::ReadStartupPreferences(os::user_paths::SettingsPath());
        Check(!preferences.automaticUpdates && preferences.uiLanguage == 2, "startup reads the portable settings path");

#ifndef _WIN32
        setenv("XDG_CONFIG_HOME", (root / "xdg").c_str(), 1);
        os::user_paths::Initialize(root / "read-only-mount");
        const auto expected = root / "xdg/lost-odyssey-recomp/settings.ini";
        Check(os::user_paths::SettingsPath() == expected, "read-only installations must use exact XDG settings file");
        Check(!fs::exists(os::user_paths::SettingsPath()), "launch-directory settings must not suppress XDG initial setup");
        WriteSettings(expected, "automatic_updates=1\nui_language=4\n");
        Check(fs::exists(os::user_paths::SettingsPath()), "saved XDG settings bypass initial setup");
        preferences = updater::ReadStartupPreferences(os::user_paths::SettingsPath());
        Check(preferences.automaticUpdates && preferences.uiLanguage == 4, "startup reads XDG settings rather than portable settings");
        fs::current_path(root / "portable");
        Check(os::user_paths::SettingsPath() == expected && fs::exists(expected), "XDG settings stay stable across launch directories");
        preferences = updater::ReadStartupPreferences(os::user_paths::SettingsPath());
        Check(preferences.automaticUpdates && preferences.uiLanguage == 4, "changed launch directory does not alter settings readback");
        unsetenv("XDG_CONFIG_HOME");
        Check(os::user_paths::SettingsPath() == fallback, "unset XDG variable uses HOME fallback");
        setenv("XDG_CONFIG_HOME", "", 1);
        Check(os::user_paths::SettingsPath() == fallback, "empty XDG variable uses HOME fallback");
#endif
        std::cout << "PASS: shared portable/XDG settings path and startup preference readback\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        result = 1;
    }
    fs::current_path(previousDirectory);
#ifndef _WIN32
    if (hadXdg) setenv("XDG_CONFIG_HOME", previousXdg.c_str(), 1);
    else unsetenv("XDG_CONFIG_HOME");
#endif
    std::error_code error;
    fs::remove_all(root, error);
    return result;
}
