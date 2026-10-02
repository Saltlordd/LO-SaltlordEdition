#pragma once

// Also selectable for host-side offline shader compilation and fixtures.
#ifndef LO_SHADER_VERTEX_BDA
#if defined(__ANDROID__)
#define LO_SHADER_VERTEX_BDA 1
#else
#define LO_SHADER_VERTEX_BDA 0
#endif
#endif

namespace xenos {
inline constexpr bool VertexFetchUsesDeviceAddress = LO_SHADER_VERTEX_BDA != 0;
inline constexpr unsigned VertexArenaAddressOffset = 1024;
}
