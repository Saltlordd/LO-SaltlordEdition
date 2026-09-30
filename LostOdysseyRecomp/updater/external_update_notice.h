#pragma once

#include <cstdint>
#include <string_view>

namespace updater
{
// Releases currently ship standalone bundles with no OSTree remote. Keep this
// guidance shared by the runtime notice, the SDL fixture, and startup logging.
inline constexpr std::string_view FlatpakReleaseUrl =
    "https://github.com/freefrank/LostOdysseyRecomp/releases/latest";
inline constexpr std::string_view FlatpakBundleInstallCommand =
    "flatpak --user install --bundle <file.flatpak>";

struct ExternalUpdateNoticeText
{
    std::string_view title;
    std::string_view download;
    std::string_view install;
    std::string_view systemScope;
    std::string_view close;
};

inline const ExternalUpdateNoticeText& FlatpakUpdateNoticeText(uint32_t uiLanguage)
{
    // Stable persisted UI IDs: EN, TW, JP, KR, SC (settings::UiLanguageNames).
    static constexpr ExternalUpdateNoticeText texts[] = {
        {"Flatpak update available",
         "Download the latest .flatpak bundle from:",
         "Install the downloaded bundle to update:",
         "For a system installation, replace --user with --system.",
         "OK (A)"},
        {"Flatpak 有可用更新",
         "請從以下頁面下載最新的 .flatpak 安裝套件：",
         "安裝下載的套件以更新：",
         "系統安裝請將 --user 改為 --system。",
         "確定 (A)"},
        {"Flatpak の更新があります",
         "以下のページから最新の .flatpak ファイルをダウンロード：",
         "ダウンロードしたファイルをインストールして更新：",
         "システムへのインストールには --user を --system に変更。",
         "閉じる (A)"},
        {"Flatpak 업데이트가 있습니다",
         "아래 페이지에서 최신 .flatpak 파일을 다운로드하세요:",
         "다운로드한 파일을 설치하여 업데이트하세요:",
         "시스템 설치에서는 --user 대신 --system을 사용하세요.",
         "확인 (A)"},
        {"Flatpak 有可用更新",
         "请从以下页面下载最新的 .flatpak 安装包：",
         "安装下载的文件以更新：",
         "系统安装请将 --user 改为 --system。",
         "确定 (A)"}
    };
    return texts[uiLanguage < 5 ? uiLanguage : 0];
}
} // namespace updater
