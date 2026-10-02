#!/usr/bin/env bash
# Build the Android vertex-BDA SPIR-V contract on a Vulkan-capable host.
set -euo pipefail

if [[ $# != 2 ]]; then
    echo "Usage: $0 <game-disc1-root> <output-directory>" >&2
    exit 2
fi

repo=$(cd "$(dirname "$0")/../.." && pwd)
game=$(cd "$1" && pwd)
output=$(mkdir -p "$2" && cd "$2" && pwd)
pack="$output/portable_vk.lospv"
image="$repo/LostOdysseyRecompLib/private/image_disc1.bin"
build=${LO_ANDROID_SHADER_HOST_BUILD_DIR:-"${XDG_CACHE_HOME:-$HOME/.cache}/lostodysseyrecomp/android-shader-pack-host"}
build=$(mkdir -p "$build" && cd "$build" && pwd)
cache=${LO_ANDROID_SHADER_HOST_CACHE_DIR:-"$build/shader-cache"}
cache=$(mkdir -p "$cache" && cd "$cache" && pwd)
run="$build/run"
zstd="$repo/tools/XenosRecomp/thirdparty/zstd"
export CC=${CC:-clang} CXX=${CXX:-clang++}

if [[ ! -f "$game/default.xex" || ! -f "$image" ]]; then
    echo "Game root must contain default.xex and the repository must contain its decrypted image_disc1.bin" >&2
    exit 1
fi
if [[ -e "$pack" ]]; then
    echo "Refusing to replace existing shader pack: $pack" >&2
    exit 1
fi
if [[ ! -f "$zstd/build/cmake/CMakeLists.txt" ]]; then
    echo "Initialize the pinned tools/XenosRecomp/thirdparty/zstd dependency before building" >&2
    exit 1
fi
cmake_sources=(-DFETCHCONTENT_SOURCE_DIR_LO_PACK_ZSTD="$zstd")
if [[ -n "${LO_ANDROID_SHADER_FFMPEG_SOURCE_DIR:-}" ]]; then
    if [[ ! -f "$LO_ANDROID_SHADER_FFMPEG_SOURCE_DIR/libavcodec/avcodec.c" ]]; then
        echo "LO_ANDROID_SHADER_FFMPEG_SOURCE_DIR must point to the pinned FFmpeg source" >&2
        exit 1
    fi
    cmake_sources+=(-DFETCHCONTENT_SOURCE_DIR_LO_FFMPEG="$LO_ANDROID_SHADER_FFMPEG_SOURCE_DIR")
fi
if [[ -n "${LO_SHADER_PACK_PATH:-}" || -n "${LO_SHADER_FULL_SCAN:-}" ||
      -n "${LO_SHADER_HLSL_DIR:-}" || -n "${LO_NO_SHADER_PREPARE:-}" ||
      -n "${LO_SHADER_RETRY_FAILURES:-}" || -n "${LO_SHADER_EXPORT_METAL:-}" ]]; then
    echo "Unset inherited shader developer overrides before building the Android pack" >&2
    exit 1
fi

cmake -S "$repo" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DLO_BUILD_RUNTIME=ON -DLO_BUILD_GPU=ON \
    -DLO_BUILD_RECOMP_LIB=ON -DLO_BUILD_TOOLS=OFF \
    -DLO_ANDROID_SHADER_PACK_HOST=ON -DSDL_PIPEWIRE=OFF "${cmake_sources[@]}"
cmake --build "$build" --target LoShaderPackTool -j "${LO_BUILD_JOBS:-4}"
tool="$build/LostOdysseyRecomp/LoShaderPackTool"
if [[ ! -x "$tool" ]]; then
    echo "Host pack tool missing from $build" >&2
    exit 1
fi

# This tool build must advertise the Android BDA contract before any expensive
# source scan or DXC run. The pack export itself is atomic; a failed preparation
# never publishes a partial file.
"$tool" contract "$image" --android > "$output/android-contract.json"
cmake --build "$build" --target LostOdysseyRecomp -j "${LO_BUILD_JOBS:-4}"
runtime="$build/LostOdysseyRecomp/LostOdysseyRecomp"
if [[ ! -x "$runtime" ]]; then
    echo "Host runtime missing from $build" >&2
    exit 1
fi
mkdir -p "$cache" "$run"
(
    cd "$run"
    LO_BACKGROUND=1 LO_NO_UPDATE=1 LO_SHADER_PACK_DOWNLOAD=0 \
    LO_GRAPHICS_API=vulkan LO_SHADER_CACHE_DIR="$cache" \
    LO_SHADER_EXPORT_PACK="$pack" \
        "$runtime" --game "$game" --prepare-shaders-only
)
"$tool" verify-runtime "$pack" "$image" --android > "$output/android-pack-report.json"
echo "Android Vulkan shader pack verified: $pack"
