#include "updater/external_update_notice.h"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main()
{
    try
    {
        using namespace updater;
        Check(FlatpakReleaseUrl == "https://github.com/freefrank/LostOdysseyRecomp/releases/latest",
              "bundle download points to official HTTPS releases page");
        Check(FlatpakBundleInstallCommand == "flatpak --user install --bundle <file.flatpak>",
              "standalone bundles update by installing a downloaded file");
        constexpr std::string_view downloadWords[] = {"Download", "下載", "ダウンロード", "다운로드", "下载"};
        constexpr std::string_view installWords[] = {"Install", "安裝", "インストール", "설치", "安装"};
        for (uint32_t language = 0; language < 5; ++language)
        {
            const auto& text = FlatpakUpdateNoticeText(language);
            Check(text.download.find(downloadWords[language]) != std::string_view::npos,
                  "every UI language explains downloading a release");
            Check(text.download.find(".flatpak") != std::string_view::npos,
                  "every UI language specifies a standalone bundle");
            Check(text.install.find(installWords[language]) != std::string_view::npos,
                  "every UI language explains installing the downloaded file");
            Check(text.systemScope.find("--user") != std::string_view::npos &&
                  text.systemScope.find("--system") != std::string_view::npos,
                  "scope guidance covers both user and system installations");
            for (const auto field : {text.title, text.download, text.install, text.systemScope, text.close})
            {
                Check(!field.empty(), "all localized notice labels are populated");
                Check(field.find("flatpak update") == std::string_view::npos,
                      "notice must not recommend an unavailable remote update");
            }
        }
        Check(&FlatpakUpdateNoticeText(5) == &FlatpakUpdateNoticeText(0) &&
              &FlatpakUpdateNoticeText(UINT32_MAX) == &FlatpakUpdateNoticeText(0),
              "unknown UI language safely falls back to English");
        std::cout << "PASS: localized standalone Flatpak bundle update guidance\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
