# Optional local Intel XeSS SDK integration (https://github.com/intel/xess).
# This file never downloads SDK content: point LO_XESS_SDK_ROOT at an extracted
# SDK containing inc/ and bin/. The headers are MIT licensed; libxess.dll,
# libxess_fg.dll and libxell.dll ship under the SDK's LICENSE.txt and are loaded
# at runtime, so a missing DLL leaves the provider unavailable instead of
# preventing startup. XeSS-SR and XeSS-FG are Windows D3D12 only here.
option(LO_ENABLE_XESS "Build Windows D3D12 Intel XeSS Super Resolution" OFF)
option(LO_ENABLE_XESS_FG "Build Windows D3D12 Intel XeSS frame generation (with XeLL)" OFF)
set(LO_XESS_SDK_ROOT "" CACHE PATH "Extracted Intel XeSS SDK 2.x root (inc/, bin/, LICENSE.txt)")

function(_lo_xess_require feature)
    if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR NOT LO_BUILD_GPU)
        message(FATAL_ERROR "${feature} requires Windows x64 and the GPU backend")
    endif()
    foreach(_file IN LISTS ARGN)
        if(NOT EXISTS "${LO_XESS_SDK_ROOT}/${_file}")
            message(FATAL_ERROR "${feature} requires LO_XESS_SDK_ROOT with ${_file}")
        endif()
    endforeach()
endfunction()

function(_lo_xess_stage target)
    foreach(_dll IN LISTS ARGN)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LO_XESS_SDK_ROOT}/bin/${_dll}" "$<TARGET_FILE_DIR:${target}>/${_dll}" VERBATIM)
    endforeach()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/licenses"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LO_XESS_SDK_ROOT}/LICENSE.txt"
            "$<TARGET_FILE_DIR:${target}>/licenses/LICENSE-XeSS.txt" VERBATIM)
endfunction()

function(lo_enable_xess target)
    if(NOT LO_ENABLE_XESS)
        target_compile_definitions(${target} PRIVATE LO_HAS_XESS=0)
        return()
    endif()
    _lo_xess_require("LO_ENABLE_XESS" inc/xess/xess_d3d12.h bin/libxess.dll LICENSE.txt)
    target_include_directories(${target} SYSTEM PRIVATE "${LO_XESS_SDK_ROOT}/inc")
    target_compile_definitions(${target} PRIVATE LO_HAS_XESS=1)
    _lo_xess_stage(${target} libxess.dll)
endfunction()

# Called from lo_enable_d3d12_fg, which owns the shared framegen_d3d12 library.
function(lo_stage_xess_fg target)
    _lo_xess_require("LO_ENABLE_XESS_FG" inc/xess_fg/xefg_swapchain_d3d12.h inc/xell/xell_d3d12.h
        bin/libxess_fg.dll bin/libxell.dll LICENSE.txt)
    _lo_xess_stage(${target} libxess_fg.dll libxell.dll)
endfunction()
