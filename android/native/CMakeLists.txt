# Asset-free platform/audio probe: real allocator + SDL + plume + XMA decoder.
# Intentionally does not replace or pretend to link LostOdysseyRecompLib.
if(NOT LO_TARGET_ISA STREQUAL "aarch64")
    message(FATAL_ERROR "Android diagnostics require arm64-v8a")
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include("${CMAKE_SOURCE_DIR}/thirdparty/ffmpeg.cmake")
set(THIRDPARTY_ROOT "${CMAKE_SOURCE_DIR}/tools/XenonRecomp/thirdparty")
add_subdirectory("${CMAKE_SOURCE_DIR}/tools/XenonRecomp/thirdparty" "${CMAKE_CURRENT_BINARY_DIR}/xenon-thirdparty")
add_subdirectory("${CMAKE_SOURCE_DIR}/tools/XenonRecomp/XenonUtils" "${CMAKE_CURRENT_BINARY_DIR}/XenonUtils")
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/SDL" "${CMAKE_CURRENT_BINARY_DIR}/SDL")
set(PLUME_SDL_VULKAN_ENABLED ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/plume" "${CMAKE_CURRENT_BINARY_DIR}/plume")
if(NOT PLUME_SDL_VULKAN_ENABLED)
    message(FATAL_ERROR "Apply tools/patches/plume-android.patch after plume-lostodyssey.patch")
endif()
target_link_libraries(plume PUBLIC SDL2::SDL2 ${CMAKE_DL_LIBS} android)
add_library(lo_android_probe SHARED
    probe.cpp
    audio_probe.cpp
    xma_decoder_probe.cpp
    xex_probe.cpp
    "${CMAKE_SOURCE_DIR}/LostOdysseyRecomp/kernel/guest_address_space.cpp")
target_include_directories(lo_android_probe PRIVATE "${CMAKE_SOURCE_DIR}/LostOdysseyRecomp")
target_link_libraries(lo_android_probe PRIVATE plume SDL2::SDL2 lo_avcodec XenonUtils log android)
target_compile_features(lo_android_probe PRIVATE cxx_std_20)
# Export SDL_main even though plume sets hidden visibility for its own directory.
set_target_properties(lo_android_probe PROPERTIES CXX_VISIBILITY_PRESET default)
# NDK r27 opt-in; all bundled ELF files must support 16 KiB loading even when
# guest-memory mapping itself reports the distinct 16 KiB alias blocker.
target_link_options(lo_android_probe PRIVATE "-Wl,-z,max-page-size=16384")
