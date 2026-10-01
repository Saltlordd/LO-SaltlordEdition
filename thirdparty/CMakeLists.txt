cmake_policy(SET CMP0077 NEW)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# o1heap's own CMakeLists only builds its tests; wrap the single source file.
add_library(o1heap STATIC "${CMAKE_CURRENT_SOURCE_DIR}/o1heap/o1heap/o1heap.c")
target_include_directories(o1heap PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/o1heap/o1heap")
# o1heap defaults to __builtin_clzl, which is 32-bit on Windows (LLP64) and corrupts every bin index.
target_compile_definitions(o1heap PRIVATE O1HEAP_CLZ=__builtin_clzll)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/SDL")
include("${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg.cmake")

# plume is added once the GPU backend lands (phase 2, rendering).
if(LO_BUILD_GPU)
    if(WIN32)
        set(PLUME_D3D12_AGILITY_SDK_ENABLED OFF CACHE BOOL "" FORCE)
    endif()
    if(UNIX AND NOT APPLE)
        set(PLUME_SDL_VULKAN_ENABLED ON CACHE BOOL "" FORCE)
    endif()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/plume")
    if(TARGET sdl_headers_copy)
        add_dependencies(plume sdl_headers_copy)
    endif()
    if(LO_TARGET_PLATFORM STREQUAL "macos")
        # plume's Metal backend translates the runtime's SPIR-V to MSL at shader
        # creation (tools/patches/plume-macos.patch). Only the MSL target is needed.
        set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Cross" EXCLUDE_FROM_ALL)
        target_link_libraries(plume PRIVATE spirv-cross-msl)
        # MetalFX spatial upscaling (Scaling filter = MetalFX); macOS 13+.
        target_link_libraries(plume PUBLIC "-framework MetalFX")
    endif()
endif()
