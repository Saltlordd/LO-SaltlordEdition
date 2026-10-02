#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

// Vendor runtime libraries (NGX, Streamline, FidelityFX) ship beside the
// executable. Linux packages keep NVIDIA's signed NGX snippet unmodified in an
// ngx/ folder there: linuxdeploy rewrites the RPATH of every ELF file directly
// in usr/bin, and NGX does not load a snippet whose signature no longer matches.
namespace os::runtime_libraries
{
    // True when `directory` holds `library` or a versioned `library.<version>`,
    // the form NGX loads on Linux (libnvidia-ngx-dlss.so.310.9.1).
    inline bool Holds(const std::filesystem::path& directory, std::string_view library)
    {
        std::error_code error;
        if (std::filesystem::is_regular_file(directory / library, error)) return true;
        // Compare UTF-8 names: a narrow string() throws on Windows for file
        // names outside the active code page.
        std::u8string versioned(library.begin(), library.end());
        versioned += u8'.';
        for (std::filesystem::directory_iterator entry(directory, error), end; !error && entry != end;
             entry.increment(error)) {
            std::error_code typeError;
            if (entry->path().filename().u8string().starts_with(versioned) && entry->is_regular_file(typeError))
                return true;
        }
        return false;
    }

    // Where to load `library` from: the executable's ngx/ folder, then the
    // executable's directory. Explicit --game launches keep the caller's
    // working directory and installed Linux layouts start in the config
    // directory, so the working directory is only the fallback; staged test
    // runs place their copies there.
    inline std::filesystem::path Find(std::string_view library, const std::filesystem::path& executableDirectory,
                                      const std::filesystem::path& workingDirectory)
    {
        if (!executableDirectory.empty())
            for (const auto& directory : {executableDirectory / "ngx", executableDirectory})
                if (Holds(directory, library)) return directory;
        return workingDirectory;
    }
}
