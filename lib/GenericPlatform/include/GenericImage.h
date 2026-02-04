//
// Created by James Miller on 2/3/2026.
//

#pragma once
#include "MacroDefs.h"
#include <vector>
#include "imgui.h"
#include "BaseTypes/Color.h"

class GenericImageResource
{
public:
    virtual ~GenericImageResource() = default;

    /// Platform/API specific image object
    void* Image = nullptr;
    /// Platform/API specific image memory object
    void* Memory = nullptr;
    /// Platform/API specific image view object
    void* ImageView = nullptr;
    /// Platform/API specific sampler object
    void* Sampler = nullptr;

    /// Platform/API specific descriptor set or texture ID
    void* DescriptorSet = nullptr;

    /// Image width in pixels
    uint32 Width = 0;
    /// Image height in pixels
    uint32 Height = 0;

    /// Image format enum value, platform/API specific
    int32 Format = 0;

    /// Image layout enum value, platform/API specific
    int32 ImageLayout = 0;

    /// Pointer to the engine instance
    void* gInstance = nullptr;

    /// Get the ImGui texture ID for this image resource
    [[nodiscard]] virtual ImTextureID GetID()
    {
        if (!bInit)
            this->RegisterTexture();
        return (ImTextureID)DescriptorSet;
    }

    /// Register the image resource as a texture with ImGui
    virtual void RegisterTexture() = 0;

    /// Destroy the image resource and free the associated memory
    virtual void DestroyTexture(void* gInstance) = 0;

    virtual void CreateCanvas(uint32 width, uint32 height, Color baseColor) = 0;

    virtual void SetClearColor(Color color) = 0;

    virtual bool WritePixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const std::vector<uint8_t>& pixels) = 0;
    virtual bool SetPixel(int x, int y, const Color& color) = 0;
    virtual bool DrawRect(int x, int y, int w, int h, const Color& color) = 0;
    virtual bool DrawLine(int x0, int y0, int x1, int y1, const Color& color, int thickness) = 0;
    virtual bool DrawFilledCircle(int cx, int cy, int radius, const Color& color) = 0;
    virtual bool DrawFilledTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Color& color) = 0;

protected:
    bool bInit = false;
};
