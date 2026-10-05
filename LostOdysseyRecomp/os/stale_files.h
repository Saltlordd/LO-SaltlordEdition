#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace os
{
// Atomic writes go to "<target name><marker>..." beside the target and are
// renamed on success. A killed process (common on Android) leaves that file
// behind for good, so the next writer of the same target removes them.
inline void RemoveStaleSiblings(const std::filesystem::path& target, std::wstring_view marker,
    const std::filesystem::path& keep = {})
{
    const auto prefix = target.filename().wstring() + std::wstring(marker);
    const auto directory = target.has_parent_path() ? target.parent_path() : std::filesystem::path(".");
    std::error_code error;
    for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error))
    {
        const auto name = it->path().filename().wstring();
        if (name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0 || it->path() == keep) continue;
        std::error_code ignored;
        std::filesystem::remove(it->path(), ignored);
    }
}
}
