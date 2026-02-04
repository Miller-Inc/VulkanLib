//
// Created by James Miller on 2/3/2026.
//

#pragma once
#include "../MacroDefs.h"
#include "Logger.h"
#if USE_VULKAN
#include "VulkanImage.h"
#define IMG_RTYPE VkImageResource
#elif USE_DRIECTX
#error DirectX support is not yet implemented
#elif USE_OPENGL
#error OpenGL support is not yet implemented
#else
#include "GenericImage.h"
#define IMG_RTYPE GenericImageResource
#endif

/// Friendly wrapper struct for an image resource, which
///     can be used in the engine and GUI. This abstracts
///     away the platform and API specific details of the
///     image resource, allowing the rest of the engine to
///     work with a consistent interface. The actual
///     implementation of the image resource is determined
///     by the platform and API being used, and is defined
///     by the IMG_RTYPE typedef. This allows for easy
///     extension to support additional platforms and
///     APIs in the future, simply by defining a new image
///     resource type and updating the IMG_RTYPE
///     typedef accordingly.
class Image
{
public:
    Image() = default;
    explicit Image(void* gInstance = nullptr)
    {
        ImageResource.gInstance = gInstance;
    }
    Image(Image& image)
    {
        ImageResource = image.ImageResource;
        ref_count = image.ref_count; // Copy reference count
    }; // Copy constructor
    ~Image() = default; // Destructor
    IMG_RTYPE ImageResource;

    uint32 ref_count = 0;
};
