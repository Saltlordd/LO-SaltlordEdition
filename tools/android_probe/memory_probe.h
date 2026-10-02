#pragma once

#include <string>

namespace lo::android_probe
{
// Probes virtual-address and small shared-memory mappings without touching the
// complete guest address space. The returned lines include a final status.
std::string ProbeMemory();
}
