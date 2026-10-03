#pragma once

// SPIR-V vertex fetch reads the vertex arena through its device address,
// stored at this offset of the shared constant block (common_hlsl.h).
namespace xenos {
inline constexpr unsigned VertexArenaAddressOffset = 1024;
}
