#if defined(__ANDROID__)
#include "http.h"

namespace updater
{
namespace
{
bool Unavailable(std::string& error)
{
    error = "Downloads are unavailable in the experimental Android build; install APK and shader updates manually";
    return false;
}
}

bool ReadResponse(std::string_view, size_t, std::string& body, std::string& error)
{
    body.clear();
    return Unavailable(error);
}

bool DownloadFile(std::string_view, const std::filesystem::path&, uint64_t,
                  const DownloadProgress&, std::string& error, bool& cancelled)
{
    cancelled = false;
    return Unavailable(error);
}

bool Download(std::string_view, const std::filesystem::path&, uint64_t,
              ProgressWindow&, std::string& error, bool& cancelled)
{
    cancelled = false;
    return Unavailable(error);
}
}
#endif
