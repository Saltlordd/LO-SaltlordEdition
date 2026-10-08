#pragma once
#include <filesystem>

enum class XexProbeResult { Missing, Rejected, Loaded };
XexProbeResult TestXexImage(const std::filesystem::path& file, void (*log)(const char*, ...));
