#pragma once

#include <string>

struct SDL_Window;

namespace lo::android_probe {

// Reports Vulkan renderer prerequisites and attempts one swapchain clear/present.
// A successful clear only proves the window and presentation path, not game rendering.
std::string ProbeVulkan(SDL_Window* window);

} // namespace lo::android_probe
