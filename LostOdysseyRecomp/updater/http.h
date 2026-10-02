#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace updater
{
class ProgressWindow;

// Called after each received chunk; returning false cancels the transfer.
using DownloadProgress = std::function<bool(uint64_t completed, uint64_t total)>;

bool ReadResponse(std::string_view url, size_t limit, std::string &body, std::string &error);
bool DownloadFile(std::string_view url, const std::filesystem::path &destination, uint64_t expectedSize,
                  const DownloadProgress &progress, std::string &error, bool &cancelled);
bool Download(std::string_view url, const std::filesystem::path &destination, uint64_t expectedSize,
              ProgressWindow &progress, std::string &error, bool &cancelled);
}
