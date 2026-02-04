//
// Created by James Miller on 2/3/2026.
//

#include "VulkanImage.h"

#if USE_VULKAN
#include <cmath>
#include <ios>
#include <iostream>
#include <sstream>

#include "imgui_impl_vulkan.h"
#include "GInstance.h"
#include "stb_image_write.h"
#include "VulkanHelpers.h"

VkImageResource::VkImageResource(void* gInstance)
{
    this->gInstance = gInstance;
    const auto GameInstance = (GInstance*)gInstance;
    if (GameInstance && GameInstance->GetVulkanSetup())
    {
        Format = VK_FORMAT_R8G8B8A8_UNORM;
        ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

void VkImageResource::RegisterTexture()
{
    if (bInit) return;

    if (Sampler == VK_NULL_HANDLE || ImageView == VK_NULL_HANDLE || Image == VK_NULL_HANDLE)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error,
            "VkImageResource::RegisterTexture: invalid handles. Sampler/View/Image may be VK_NULL_HANDLE. "
            "Sampler=" + std::to_string((uint64_t)Sampler) +
            " ImageView=" + std::to_string((uint64_t)ImageView) +
            " Image=" + std::to_string((uint64_t)Image));
        return;
    }

    // Attempt to create ImGui descriptor
    DescriptorSet = ImGui_ImplVulkan_AddTexture(Sampler, ImageView, ImageLayout);

    // Always log the returned descriptor and handles (hex is easier to read for Vulkan handles)
    {
        std::ostringstream oss;
        oss << "VkImageResource::RegisterTexture: AddTexture returned descriptor = 0x"
            << std::hex << (uint64_t)DescriptorSet << std::dec
            << "  Sampler=0x" << std::hex << (uint64_t)Sampler
            << " ImageView=0x" << std::hex << (uint64_t)ImageView
            << " Image=0x" << std::hex << (uint64_t)Image;
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Info, oss.str());
    }

    if (DescriptorSet == 0)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error,
            "VkImageResource::RegisterTexture: ImGui_ImplVulkan_AddTexture returned 0 (failed to allocate descriptor).");
        bInit = false;
        return;
    }

    bInit = true;
}

void VkImageResource::DestroyTexture(void* gInstance)
{
    if (bInit)
    {
        ImGui_ImplVulkan_RemoveTexture(DescriptorSet);
        bInit = false;
    }

    const GInstance* GameInstance = (GInstance*)gInstance;

    if (!GameInstance)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Warning,
            "VkImageResource::DestroyTexture: GInstance is null, cannot destroy Vulkan resources.");
        return;
    }

    if (const auto vSetup = GameInstance->GetVulkanSetup())
    {
        if (ImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(vSetup->device, ImageView, nullptr);
            ImageView = VK_NULL_HANDLE;
        }

        if (Image != VK_NULL_HANDLE) {
            vkDestroyImage(vSetup->device, Image, nullptr);
            Image = VK_NULL_HANDLE;
        }

        if (Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(vSetup->device, Memory, nullptr);
            Memory = VK_NULL_HANDLE;
        }
    }
}

void VkImageResource::CreateCanvas(const uint32 width, const uint32 height, const Color baseColor)
{
    if (!CreateBlankCanvas(width, height))
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error,
            "VkImageResource::CreateCanvas: Failed to create blank canvas of size " +
            std::to_string(width) + "x" + std::to_string(height));
        return;
    }
    SetClearColor(baseColor);
    RegisterTexture();
}

void VkImageResource::SetClearColor(const Color color)
{

    if (!gInstance)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Warning,
            "VkImageResource::CreateBlankCanvas: GInstance is null, cannot create Vulkan resources.");
        return;
    }

    SetupSampler();

    const GInstance* GameInstance = (GInstance*)gInstance;

    const VkCommandBuffer cmd = beginSingleUseCommands(
        GameInstance->GetVulkanSetup()->device,
        GameInstance->GetVulkanSetup()->commandPool);

    // 1. Transition to TRANSFER_DST so we can write to it
    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // 2. Clear the image
    auto [r, g, b, a] = color.fRGBA();
    const VkClearColorValue clearColor = {r, g, b, a};
    constexpr VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdClearColorImage(cmd, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

    // 3. Transition back to SHADER_READ_ONLY for ImGui
    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endSingleUseCommands(GameInstance->GetVulkanSetup()->device,
        GameInstance->GetVulkanSetup()->queue, GameInstance->GetVulkanSetup()->commandPool, cmd);
}

bool VkImageResource::CreateBlankCanvas(const uint32_t width, const uint32_t height) {
    Width = width;
    Height = height;

    if (!gInstance)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Warning,
            "VkImageResource::CreateBlankCanvas: GInstance is null, cannot create Vulkan resources.");
        return false;
    }
    const auto* GameInstance = (GInstance*)gInstance;

    const VkDevice device = GameInstance->GetVulkanSetup()->device;
    const VkPhysicalDevice physDevice = GameInstance->GetVulkanSetup()->physicalDevice;
    const VkCommandPool commandPool = GameInstance->GetVulkanSetup()->commandPool;
    const VkQueue graphicsQueue = GameInstance->GetVulkanSetup()->queue;

    // 1. Create VkImage
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = Format;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageInfo, nullptr, &Image) != VK_SUCCESS) return false;

    // 2. Memory Allocation (Simplified - using VMA is highly recommended here)
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, Image, &memReqs);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &Memory) != VK_SUCCESS) return false;
    vkBindImageMemory(device, Image, Memory, 0);

    // 3. Create Image View
    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = Format;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    if (vkCreateImageView(device, &viewInfo, nullptr, &ImageView) != VK_SUCCESS) return false;

    // 4. Create Sampler
    VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &Sampler) != VK_SUCCESS) return false;

    // 5. Transition Layout to SHADER_READ_ONLY_OPTIMAL
    TransitionImageLayout(device, commandPool, graphicsQueue, Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // 6. Register with ImGui
    // DescriptorSet = ImGui_ImplVulkan_AddTexture(Sampler, ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return true;
}

void VkImageResource::SetupSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler sampler;
    if (vkCreateSampler(((GInstance*)gInstance)->GetVulkanSetup()->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "Failed to create sampler!");
        return ;
    }

    Sampler = sampler;
}

void VkImageResource::DumpDebugInfo() const
{
    std::cout << "VkImageResource::DumpDebugInfo: bInit=" << (bInit ? "true" : "false")
        << " Descriptor=0x" << std::hex << (uint64_t)DescriptorSet
        << " Sampler=0x" << std::hex << (uint64_t)Sampler
        << " ImageView=0x" << std::hex << (uint64_t)ImageView
        << " Image=0x" << std::hex << (uint64_t)Image
        << " Format=" << std::dec << Format;
    M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Info, oss.str());
}

void VkImageResource::DumpToPNG(const std::string& path) const
{
    if (!gInstance)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Warning, "DumpToPNG: GInstance is null.");
        return;
    }
    const GInstance* GameInstance = (GInstance*)gInstance;
    const auto vSetup = GameInstance->GetVulkanSetup();
    if (!vSetup)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Warning, "DumpToPNG: Vulkan setup missing.");
        return;
    }

    VkDevice device = vSetup->device;
    VkPhysicalDevice phys = vSetup->physicalDevice;
    VkCommandPool cmdPool = vSetup->commandPool;
    VkQueue queue = vSetup->queue;

    if (Width == 0 || Height == 0)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Warning, "DumpToPNG: zero-sized image.");
        return;
    }

    VkDeviceSize imageSize = VkDeviceSize(Width) * VkDeviceSize(Height) * 4; // RGBA8

    // 1) Create staging buffer (TRANSFER_DST, HOST visible)
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = imageSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error, "DumpToPNG: vkCreateBuffer failed.");
        return;
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(phys, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS)
    {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error, "DumpToPNG: vkAllocateMemory failed.");
        return;
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // 2) Record command: transition -> copy -> transition back
    VkCommandBuffer cmd = beginSingleUseCommands(device, cmdPool);

    // Transition image to TRANSFER_SRC
    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { Width, Height, 1 };

    vkCmdCopyImageToBuffer(cmd, Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    // Transition back to shader read-only
    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endSingleUseCommands(device, queue, cmdPool, cmd);

    // 3) Map and save PNG
    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped);
    if (!mapped)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error, "DumpToPNG: vkMapMemory failed.");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return;
    }

    std::vector<uint8_t> pixels;
    pixels.resize(static_cast<size_t>(imageSize));
    memcpy(pixels.data(), mapped, (size_t)imageSize);

    // If your image format is BGR(A) you will see channel swaps here.
    int writeResult = stbi_write_png(path.c_str(), (int)Width, (int)Height, 4, pixels.data(), (int)(Width * 4));
    vkUnmapMemory(device, stagingMemory);

    // cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    if (writeResult)
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Info, std::string("DumpToPNG: wrote ") + path);
    }
    else
    {
        M_LOGGER(Logger::LogGraphics, Logger::LogLevel::Error, std::string("DumpToPNG: failed to write ") + path);
    }
}

static inline uint8_t float_to_u8(float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    return static_cast<uint8_t>(std::lround(v * 255.0f));
}

bool VkImageResource::WritePixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const std::vector<uint8_t>& pixels)
{
    if (!gInstance || !Image || pixels.empty()) return false;
    if (w == 0 || h == 0) return false;
    if (pixels.size() < (size_t)w * h * 4) return false; // expect RGBA8

    const GInstance* GameInstance = (GInstance*)gInstance;
    const auto vSetup = GameInstance->GetVulkanSetup();
    if (!vSetup) return false;

    // Clip region to canvas
    if (x >= Width || y >= Height) return false;
    uint32_t copyW = std::min<uint32_t>(w, Width - x);
    uint32_t copyH = std::min<uint32_t>(h, Height - y);

    VkDevice device = vSetup->device;
    VkPhysicalDevice phys = vSetup->physicalDevice;
    VkCommandPool cmdPool = vSetup->commandPool;
    VkQueue queue = vSetup->queue;

    VkDeviceSize uploadSize = VkDeviceSize(copyW) * VkDeviceSize(copyH) * 4;

    // Create staging buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = uploadSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuffer) != VK_SUCCESS) return false;

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(phys, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return false;
    }
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Map and copy only the portion requested (rows tightly packed)
    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, uploadSize, 0, &mapped);
    if (!mapped) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }

    // If input pixels represent full w x h but we clipped, we need to copy row by row
    if (copyW == w && copyH == h) {
        memcpy(mapped, pixels.data(), static_cast<size_t>(uploadSize));
    } else {
        const uint8_t* src = pixels.data();
        uint8_t* dst = reinterpret_cast<uint8_t*>(mapped);
        size_t srcRowBytes = (size_t)w * 4;
        size_t dstRowBytes = (size_t)copyW * 4;
        for (uint32_t row = 0; row < copyH; ++row) {
            const uint8_t* srcRow = src + (size_t)row * srcRowBytes;
            memcpy(dst + (size_t)row * dstRowBytes, srcRow, dstRowBytes);
        }
    }
    vkUnmapMemory(device, stagingMemory);

    // Record command: transition -> copy -> transition back
    VkCommandBuffer cmd = beginSingleUseCommands(device, cmdPool);

    // Transition image to TRANSFER_DST
    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { static_cast<int32_t>(x), static_cast<int32_t>(y), 0 };
    region.imageExtent = { copyW, copyH, 1 };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition back to shader read-only
    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endSingleUseCommands(device, queue, cmdPool, cmd);

    // cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    return true;
}

bool VkImageResource::SetPixel(int x, int y, const Color& color)
{
    if (x < 0 || y < 0) return false;
    if ((uint32_t)x >= Width || (uint32_t)y >= Height) return false;
    auto [rf, gf, bf, af] = color.fRGBA();
    std::vector<uint8_t> px = {
        float_to_u8(rf), float_to_u8(gf), float_to_u8(bf), float_to_u8(af)
    };
    return WritePixels((uint32_t)x, (uint32_t)y, 1, 1, px);
}

bool VkImageResource::DrawRect(int x, int y, int w, int h, const Color& color)
{
    if (w <= 0 || h <= 0) return false;
    // Clip will be handled by WritePixels
    auto [rf, gf, bf, af] = color.fRGBA();
    uint8_t r = float_to_u8(rf), g = float_to_u8(gf), b = float_to_u8(bf), a = float_to_u8(af);
    std::vector<uint8_t> pixels;
    pixels.resize((size_t)w * (size_t)h * 4);
    for (size_t i = 0; i < (size_t)w * (size_t)h; ++i) {
        pixels[i*4 + 0] = r;
        pixels[i*4 + 1] = g;
        pixels[i*4 + 2] = b;
        pixels[i*4 + 3] = a;
    }
    return WritePixels((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, pixels);
}

bool VkImageResource::DrawLine(int x0, int y0, int x1, int y1, const Color& color, int thickness)
{
    if (!gInstance || !Image) return false;
    if (thickness <= 0) return false;

    // Clip trivial invalids
    if (Width == 0 || Height == 0) return false;

    // Bresenham integer line traversal but we only collect pixels (clipped)
    // Collect per-row x coordinates so we can merge contiguous runs later.
    std::unordered_map<int, std::vector<int>> rows;
    rows.reserve(static_cast<size_t>(std::abs(y1 - y0) + 1));

    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int cx = x0;
    int cy = y0;

    // radius as float so thickness can be even/odd and produce smoother disk
    const float radius = std::max(0.5f, thickness * 0.5f);
    const int rceil = (int)std::ceil(radius);

    while (true) {
        // Expand point into a disk of radius and add only clipped pixels
        for (int oy = -rceil; oy <= rceil; ++oy) {
            int ny = cy + oy;
            if (ny < 0 || (uint32_t)ny >= Height) continue;
            // compute dx extent for this row for the floating radius
            float fy = static_cast<float>(oy);
            float remain = radius * radius - fy * fy;
            if (remain < 0.0f) continue;
            int dxMax = (int)std::floor(std::sqrt(remain));
            for (int ox = -dxMax; ox <= dxMax; ++ox) {
                int nx = cx + ox;
                if (nx < 0 || (uint32_t)nx >= Width) continue;
                rows[ny].push_back(nx);
            }
        }

        if (cx == x1 && cy == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            cx += sx;
        }
        if (e2 <= dx) {
            err += dx;
            cy += sy;
        }
    }

    if (rows.empty()) return false;

    // Convert color once
    auto [rf, gf, bf, af] = color.fRGBA();
    uint8_t rc = float_to_u8(rf), gc = float_to_u8(gf), bc = float_to_u8(bf), ac = float_to_u8(af);

    // Build merged horizontal segments from per-row x lists
    struct Seg { uint32_t x; uint32_t y; uint32_t w; };
    std::vector<Seg> segments;
    segments.reserve(rows.size());
    size_t totalBytes = 0;

    for (auto &kv : rows) {
        int y = kv.first;
        auto &xs = kv.second;
        if (xs.empty()) continue;
        std::sort(xs.begin(), xs.end());
        xs.erase(std::unique(xs.begin(), xs.end()), xs.end());

        int start = xs[0];
        int prev = xs[0];
        for (size_t i = 1; i < xs.size(); ++i) {
            if (xs[i] == prev + 1) {
                prev = xs[i];
                continue;
            }
            // flush segment [start..prev]
            uint32_t sxu = static_cast<uint32_t>(start);
            uint32_t wu = static_cast<uint32_t>(prev - start + 1);
            segments.push_back({sxu, static_cast<uint32_t>(y), wu});
            totalBytes += (size_t)wu * 4;
            start = xs[i];
            prev = xs[i];
        }
        // flush last segment
        uint32_t sxu = static_cast<uint32_t>(start);
        uint32_t wu = static_cast<uint32_t>(prev - start + 1);
        segments.push_back({sxu, static_cast<uint32_t>(y), wu});
        totalBytes += (size_t)wu * 4;
    }

    if (segments.empty()) return false;

    // Vulkan setup handles
    const GInstance* GameInstance = (GInstance*)gInstance;
    const auto vSetup = GameInstance->GetVulkanSetup();
    if (!vSetup) return false;

    VkDevice device = vSetup->device;
    VkPhysicalDevice phys = vSetup->physicalDevice;
    VkCommandPool cmdPool = vSetup->commandPool;
    VkQueue queue = vSetup->queue;

    // Create staging buffer sized to hold all segments packed sequentially
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkDeviceSize uploadSize = (VkDeviceSize)totalBytes;

    VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = uploadSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuffer) != VK_SUCCESS) return false;

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(phys, memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return false;
    }
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Map and fill packed pixel data for each segment
    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, uploadSize, 0, &mapped);
    if (!mapped) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    uint8_t* writePtr = reinterpret_cast<uint8_t*>(mapped);
    for (const Seg& s : segments) {
        for (size_t i = 0; i < (size_t)s.w; ++i) {
            writePtr[0] = rc;
            writePtr[1] = gc;
            writePtr[2] = bc;
            writePtr[3] = ac;
            writePtr += 4;
        }
    }
    vkUnmapMemory(device, stagingMemory);

    // Prepare buffer->image copy regions, buffer offsets correspond to packed segments
    std::vector<VkBufferImageCopy> regions;
    regions.reserve(segments.size());
    VkDeviceSize curOffset = 0;
    for (const Seg& s : segments) {
        VkBufferImageCopy region{};
        region.bufferOffset = curOffset;
        region.bufferRowLength = 0;   // tightly packed
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { (int32_t)s.x, (int32_t)s.y, 0 };
        region.imageExtent = { s.w, 1, 1 };
        regions.push_back(region);
        curOffset += (VkDeviceSize)s.w * 4;
    }

    // Record and submit single-use command buffer: transition -> copy many regions -> transition back
    VkCommandBuffer cmd = beginSingleUseCommands(device, cmdPool);

    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkCmdCopyBufferToImage(cmd, stagingBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        (uint32_t)regions.size(), regions.data());

    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endSingleUseCommands(device, queue, cmdPool, cmd);

    // Cleanup staging resources
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    return true;
}

bool VkImageResource::DrawFilledCircle(int cx, int cy, int radius, const Color& color)
{
    if (!gInstance || !Image) return false;
    if (radius <= 0) return false;
    if (Width == 0 || Height == 0) return false;

    const GInstance* GameInstance = (GInstance*)gInstance;
    const auto vSetup = GameInstance->GetVulkanSetup();
    if (!vSetup) return false;

    VkDevice device = vSetup->device;
    VkPhysicalDevice phys = vSetup->physicalDevice;
    VkCommandPool cmdPool = vSetup->commandPool;
    VkQueue queue = vSetup->queue;

    int x0 = cx - radius;
    int y0 = cy - radius;
    int x1 = cx + radius;
    int y1 = cy + radius;

    // Clip to canvas bounds
    int destX = std::max(0, x0);
    int destY = std::max(0, y0);
    int endX = std::min((int)Width - 1, x1);
    int endY = std::min((int)Height - 1, y1);
    if (destX > endX || destY > endY) return false;

    // Convert color once
    auto [rf, gf, bf, af] = color.fRGBA();
    uint8_t rc = float_to_u8(rf), gc = float_to_u8(gf), bc = float_to_u8(bf), ac = float_to_u8(af);

    // Build list of horizontal segments (x_start, y, width) that are inside the circle
    struct Seg { uint32_t x; uint32_t y; uint32_t w; };
    std::vector<Seg> segments;
    segments.reserve((endY - destY + 1));

    size_t totalBytes = 0;
    int rr = radius * radius;
    for (int y = destY; y <= endY; ++y) {
        int dy = y - cy;
        int dxMax = (int)std::floor(std::sqrt((double)rr - (double)dy * (double)dy));
        int sx = cx - dxMax;
        int ex = cx + dxMax;
        // clip segment to canvas
        sx = std::max(sx, destX);
        ex = std::min(ex, endX);
        if (sx <= ex) {
            uint32_t w = (uint32_t)(ex - sx + 1);
            segments.push_back({ (uint32_t)sx, (uint32_t)y, w });
            totalBytes += (size_t)w * 4;
        }
    }

    if (segments.empty()) return false;

    // Create staging buffer sized to hold all segments packed sequentially
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkDeviceSize uploadSize = (VkDeviceSize)totalBytes;

    VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = uploadSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuffer) != VK_SUCCESS) return false;

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(phys, memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return false;
    }
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Map and fill packed pixel data for each segment
    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, uploadSize, 0, &mapped);
    if (!mapped) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    uint8_t* writePtr = reinterpret_cast<uint8_t*>(mapped);
    for (const Seg& s : segments) {
        size_t segBytes = (size_t)s.w * 4;
        // fill with color
        for (size_t i = 0; i < (size_t)s.w; ++i) {
            writePtr[0] = rc;
            writePtr[1] = gc;
            writePtr[2] = bc;
            writePtr[3] = ac;
            writePtr += 4;
        }
    }
    vkUnmapMemory(device, stagingMemory);

    // Prepare buffer->image copy regions, buffer offsets correspond to packed segments
    std::vector<VkBufferImageCopy> regions;
    regions.reserve(segments.size());
    VkDeviceSize curOffset = 0;
    for (const Seg& s : segments) {
        VkBufferImageCopy region{};
        region.bufferOffset = curOffset;
        region.bufferRowLength = 0;   // tightly packed
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { (int32_t)s.x, (int32_t)s.y, 0 };
        region.imageExtent = { s.w, 1, 1 };
        regions.push_back(region);
        curOffset += (VkDeviceSize)s.w * 4;
    }

    // Record and submit single-use command buffer: transition -> copy many regions -> transition back
    VkCommandBuffer cmd = beginSingleUseCommands(device, cmdPool);

    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkCmdCopyBufferToImage(cmd, stagingBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        (uint32_t)regions.size(), regions.data());

    TransitionImageLayoutInline(cmd, Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endSingleUseCommands(device, queue, cmdPool, cmd);

    // Cleanup staging resources
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    return true;
}

#endif
