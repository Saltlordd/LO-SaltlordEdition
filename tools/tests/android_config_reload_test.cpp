// Exercise the production settings singleton across a bootstrap path change.
#include <stdafx.h>
#include <os/logger.h>
#include <gpu/dlss_status_log.h>
#undef LOG_INFO
#define LOG_INFO(...) ((void)0)
#undef LOG_WARNING
#define LOG_WARNING(...) ((void)0)
#include "../../LostOdysseyRecomp/settings/config.cpp"
int main(int argc,char** argv) {
    assert(argc==2);
    const auto root=std::filesystem::absolute(argv[1]);
    std::filesystem::create_directories(root/"before");
    std::filesystem::create_directories(root/"after");
    std::filesystem::current_path(root/"before");
    const auto early=settings::GetConfig();
    assert(early.scalingQuality==1 && early.internalResolution==0);
    std::ofstream(root/"after/settings.ini") << "scaling_quality=0\ninternal_resolution=720\nframe_rate=60\nexpand_rgb_range=1\nmotion_blur=0\ndynamic_shadows=0\n";
    std::filesystem::current_path(root/"after");
    assert(settings::GetConfig().scalingQuality==1);
    settings::ReloadConfig();
    const auto loaded=settings::GetConfig();
    assert(loaded.scalingQuality==0 && loaded.internalResolution==720 && loaded.frameRate==60);
    assert(loaded.expandRgbRange && !loaded.motionBlur && !loaded.dynamicShadows);
    assert(settings::SaveConfig(loaded));
    settings::ReloadConfig();
    assert(settings::GetConfig()==loaded);
    std::puts("PASS: early cached desktop defaults replaced by complete persisted config; save/reload preserves filter, colour, target and effects");
}
