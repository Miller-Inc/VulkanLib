//
// Created by James Miller on 2/3/2026.
//

#pragma once
#include "MacroDefs.h"

#if USE_VULKAN
#include <string>
#include <vector>

#include "GenericImage.h"
#include "VulkanInclude.h"
#include "imgui.h"


class VkImageResource : public GenericImageResource
{
public:
    VkImageResource() = default;
    explicit VkImageResource(void* gInstance);
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkSampler Sampler = VK_NULL_HANDLE;

    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

    uint32_t Width = 0;
    uint32_t Height = 0;

    VkFormat Format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    void RegisterTexture() override;

    void DestroyTexture(void* gInstance) override;

    void CreateCanvas(uint32 width, uint32 height, Color baseColor) override;

    void SetClearColor(Color color) override;

    void DumpDebugInfo() const;
    void DumpToPNG(const std::string& path) const;

    ImTextureID GetID() override
    {
        return (ImTextureID)DescriptorSet;
    }

    bool WritePixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const std::vector<uint8_t>& pixels) override;
    bool SetPixel(int x, int y, const Color& color) override;
    bool DrawRect(int x, int y, int w, int h, const Color& color) override;
    bool DrawLine(int x0, int y0, int x1, int y1, const Color& color, int thickness)override;
    bool DrawFilledCircle(int cx, int cy, int radius, const Color& color) override;

private:
    bool CreateBlankCanvas(uint32_t width, uint32_t height);
    void SetupSampler();
};

#endif