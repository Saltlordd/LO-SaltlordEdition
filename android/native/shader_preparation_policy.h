#pragma once
#include <cstdlib>
#include <initializer_list>
inline void ApplyAndroidShaderPreparation(bool prepare) {
    for(const char* flag:{"LO_NO_SHADER_PREPARE","LO_NO_PIPELINE_PREPARE","LO_NO_PORTABLE_SHADER_PACK"}) {
        if(prepare)unsetenv(flag);else setenv(flag,"1",1);
    }
}
