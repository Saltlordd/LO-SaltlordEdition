#include <settings/android_test_baseline.h>
#include <cassert>
#include <cstdio>
int main(){
 settings::Config high;high.skipShaderPrebuild=true;high.internalResolution=2160;high.antialiasing=3;high.fxaa=true;
 high.anisotropicFiltering=16;high.scalingQuality=1;high.upscaler=gpu::upscaling::Upscaler::Fsr;
 high.fsrSharpnessPercent=100;high.frameGenerationProvider=framegen::Provider::Fsr;
 high.frameRate=120;high.variableRefreshRate=true;high.uiLanguage=2;high.gameLanguage=4;high.saveAnywhere=true;
 auto low=settings::AndroidLowGraphicsBaseline(high);
 assert(!low.skipShaderPrebuild);
 assert(low.internalResolution==720&&low.width==1280&&low.height==720);
 assert(low.antialiasing==0&&!low.fxaa&&low.anisotropicFiltering==0&&low.scalingQuality==0);
 assert(low.upscaler==gpu::upscaling::Upscaler::Off&&low.fsrSharpnessPercent==0);
 assert(low.frameGenerationProvider==framegen::Provider::Off&&low.frameRate==30&&!low.variableRefreshRate);
 assert(low.uiLanguage==2&&low.gameLanguage==4&&low.saveAnywhere);
 auto edited=low;edited.antialiasing=2;assert(edited.antialiasing==2);
 assert(settings::AndroidLowGraphicsBaseline(edited)==low);
 std::puts("PASS: lowest baseline, unrelated preferences retained, live edits possible, fresh launch reset");
}
