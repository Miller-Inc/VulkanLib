//
// Created by James Miller on 11/29/2025.
//

#pragma once
#include "WindowBase.h"
#include "GenericImage.h"
#include "GInstance.h"
#include "BaseTypes/Image.h"
#include "VulkanViewport.h"

class Viewport final : public WindowBase
{
public:
    Viewport();
    Viewport(const Viewport& other);
    ~Viewport() override = default;
    void Init() override;
    void Init(const std::string& WindowName, GInstance* Instance) override;
    void Init(GInstance* GameInstance) override;
    void Open() override;
    void Draw(float deltaTime) override;
    void Close() override;

    /// Tick function for updating physics and rendering
    void Tick(float deltaTime) override;

    std::string Name = "Viewport";

private:

    float avg_tick_time = 0.0f; // Average time per tick for performance monitoring

    Image mImage = Image(nullptr); // default-construct; will be initialized when Viewport::Init is called

    // Mouse capture state for viewport input handling
    bool mMouseCaptured = false;
    float mMouseSensitivity = 0.005f; // radians per pixel (tweak as needed)
    bool mPrevEscDown = false;
    float mMoveSpeed = 5.0f; // units per second

    void GenerateCanvas();
    void GenerateCleanCanvas();
    void TestRender();

    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

    // Added for SSBO demonstration
    VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
    VkBuffer SSBOBuffer = VK_NULL_HANDLE;
    VkDeviceMemory SSBOBufferMemory = VK_NULL_HANDLE;
    void* SSBOMappedPtr = nullptr; // mapped pointer for HOST_VISIBLE SSBO
    VkDeviceSize SSBOSize = 0;

    VulkanViewport mViewport;

    bool bEditorOpen = true;

    // CPU-driven triangle translation (NDC space)
    float mTriX = 0.0f;
    float mTriY = 0.0f;
};
