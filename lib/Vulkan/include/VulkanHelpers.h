//
// Created by James Miller on 11/30/2025.
//

#pragma once
#include "MacroDefs.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "imgui.h"

struct VulkanResource {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    void* mappedData;
};

void DestroyVulkanHelpersResources(VkDevice device);
VkBuffer createStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkDeviceMemory* outMem);
void createOrResizeImage(VkDevice device, VkPhysicalDevice phys, int w, int h, VkFormat fmt, VkImage* outImage, VkDeviceMemory* outMemory, VkImageView* outView = nullptr);
VkCommandBuffer beginSingleUseCommands(VkDevice device, VkCommandPool pool);
void endSingleUseCommands(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer cmd);
void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat fmt, VkImageLayout oldLayout, VkImageLayout newLayout);
void copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image, uint32_t w, uint32_t h);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat fmt);
VkSampler GetOrCreateSampler(VkDevice device);
uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void TransitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void TransitionImageLayoutInline(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

std::vector<int8> LoadShaderCode(const std::string& filename);
VkShaderModule CreateShaderModule(VkDevice device, const std::vector<int8>& code);

void* CreateAndMapBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

bool MapVulkanMemoryLocation(uint32 memory_location, const VkBuffer& buffer, const VkDevice& device, const VkDescriptorSet& descriptorSet);

VulkanResource CreateMappedStorageBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize& size, uint32_t bindingPoint);

void BindPipelineToBuffer(const VkCommandBuffer& cmd, const VkPipelineLayout& pipeline_layout, const VulkanResource& resource);
