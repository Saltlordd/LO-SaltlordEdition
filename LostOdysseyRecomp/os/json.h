#pragma once

#if defined(__ANDROID__)
#include <nlohmann/json.hpp>
#else
#include "../../tools/XenonRecomp/thirdparty/tomlplusplus/vendor/json.hpp"
#endif
