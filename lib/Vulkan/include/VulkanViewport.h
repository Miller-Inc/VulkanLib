//
// Created by James Miller on 2/10/2026.
//

#pragma once
#include "MacroDefs.h"
#include "VulkanImage.h"
#include <chrono>

#include "VertexInfo.h"
#include "VulkanHelpers.h"

class GInstance; // Forward declaration of GInstance

typedef struct
{
    float32 x, y, z;
} float3;

struct MeshPushConstants {
    float time;
    float r, g, b;
    float3 positions[6];
    float3 normals[6];
};

typedef struct
{
    float32 u, v;
} float2;


struct Vec4 { float x,y,z,w; };

enum ViewportType : uint8
{
    VIEWPORT_NOT_INITIALIZED = 0,
    VIEWPORT_NO_SHADERS = 1,
    VIEWPORT_TEST_SHADERS = 2,
    VIEWPORT_SHADERS = 3,
};

class VulkanViewport
{
public:
    VulkanViewport() = default;
    explicit VulkanViewport(void* gInstance);
    VulkanViewport(VulkanViewport const& Other) = delete; // disallow copy to avoid shallow-copying Vulkan handles
    VulkanViewport(void* gInstance, int32 Width, int32 Height, uint8 Type = VIEWPORT_NO_SHADERS);
    ~VulkanViewport();
    void Cleanup();

    void GenerateCanvas();
    void GenerateRenderTarget();
    void Render();

    // Copy 3 vertex positions (Vec4) into the SSBO from CPU. Safe no-op if SSBO not created.
    void SetTrianglePositions(const float3* verts, size_t count) const;
    void SetTriangleIndices(const uint32* indices, size_t count) const;
    void SetVertexInfo(const VertexInfo* vertexInfo, size_t count) const;

    VkImageResource ImageResource;
    int32 Width = 1300, Height = 780;

    VkDescriptorSet GetDescriptorSet() const;
    void UpdateGInstance(GInstance* newInstance);


    GInstance* mInstance = nullptr;
    uint8 Type = VIEWPORT_NOT_INITIALIZED;
    bool bInit = false;
    // Debug id to help track multiple instances at runtime
    uint64_t mId = 0;
    bool mImageCreated = false;

    // Cached render-target handles copied after successful CreateRenderTarget
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
    uint32_t mImageWidth = 0;
    uint32_t mImageHeight = 0;

    VulkanResource VertResource{};
    VulkanResource TriResource{};


    VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
    VkBuffer SSBOBuffer = VK_NULL_HANDLE;
    VkDeviceMemory SSBOBufferMemory = VK_NULL_HANDLE;
    void* SSBOMappedPtr = nullptr; // mapped pointer for HOST_VISIBLE SSBO
    VkBuffer TriBuffer = VK_NULL_HANDLE;
    VkDeviceMemory TriBufferMemory = VK_NULL_HANDLE;
    void* TriangleBufferPtr = nullptr;
    VkDeviceSize SSBOSize = 0;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
};
