#pragma once

#include <cstddef>

// Image prefix that identifies the executable to portable shader packs. Its
// bytes are taken as parsed, before XexLoader binds imports, so they match the
// same prefix of tools/xexdump output and do not depend on host addresses.
namespace xex_identity
{
    inline constexpr size_t PrefixBytes = 0x185C60;
}
