//
// Created by James Miller on 2/4/2026.
//

#pragma once
#include "MacroDefs.h"

#if USE_VULKAN
#if PLATFORM_WINDOWS
    #include <Vulkan/vulkan.h>
#elif PLATFORM_UNIX
#include <vulkan/vulkan.h>
#else
    #error Unsupported platform for Vulkan
#endif
#endif