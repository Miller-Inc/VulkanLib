//
// Created by James Miller on 11/30/2025.
//

#include "../include/VulkanHelpers.h"
#include <cstdio>
#include <vector>
#include <cassert>
#include <cstring>
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "Logger.h"

static uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(phys, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    std::fprintf(stderr, "findMemoryType: suitable memory type not found\n");
    return UINT32_MAX;
}

VkBuffer createStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkDeviceMemory* outMem)
{
    VkBufferCreateInfo bufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = size;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buf = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufInfo, nullptr, &buf) != VK_SUCCESS) {
        std::fprintf(stderr, "createStagingBuffer: vkCreateBuffer failed\n");
        return VK_NULL_HANDLE;
    }

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buf, &req);

    uint32_t memType = findMemoryType(physicalDevice, req.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memType == UINT32_MAX) {
        vkDestroyBuffer(device, buf, nullptr);
        return VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memType;

    VkDeviceMemory mem = VK_NULL_HANDLE;
    if (vkAllocateMemory(device, &alloc, nullptr, &mem) != VK_SUCCESS) {
        std::fprintf(stderr, "createStagingBuffer: vkAllocateMemory failed\n");
        vkDestroyBuffer(device, buf, nullptr);
        return VK_NULL_HANDLE;
    }

    vkBindBufferMemory(device, buf, mem, 0);
    if (outMem) *outMem = mem;
    return buf;
}

void createOrResizeImage(VkDevice device, VkPhysicalDevice phys, int w, int h, VkFormat fmt, VkImage* outImage, VkDeviceMemory* outMemory, VkImageView* outView /*= nullptr*/)
{
    if (!outImage || !outMemory) return;

    // Ensure all GPU work that might reference the old image/view has completed.
    // This is coarse (blocks until idle) but prevents "image in use by VkImageView" and
    // "bound memory was freed" validation errors. Replace with proper lifetime
    // management for better performance later.
    vkDeviceWaitIdle(device);

    // Destroy existing image view first (views reference the image).
    if (outView && *outView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, *outView, nullptr);
        *outView = VK_NULL_HANDLE;
    }

    // Now safe to destroy the image and free memory.
    if (*outImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, *outImage, nullptr);
        *outImage = VK_NULL_HANDLE;
    }
    if (*outMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, *outMemory, nullptr);
        *outMemory = VK_NULL_HANDLE;
    }

    // Create new image
    VkImageCreateInfo imgInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.extent.width = uint32_t(w);
    imgInfo.extent.height = uint32_t(h);
    imgInfo.extent.depth = 1;
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.format = fmt;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT
        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imgInfo, nullptr, outImage) != VK_SUCCESS) {
        std::fprintf(stderr, "createOrResizeImage: vkCreateImage failed\n");
        *outImage = VK_NULL_HANDLE;
        return;
    }

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, *outImage, &req);

    uint32_t memType = findMemoryType(phys, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    assert(memType != UINT32_MAX);

    VkMemoryAllocateInfo alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memType;

    if (vkAllocateMemory(device, &alloc, nullptr, outMemory) != VK_SUCCESS) {
        std::fprintf(stderr, "createOrResizeImage: vkAllocateMemory failed\n");
        vkDestroyImage(device, *outImage, nullptr);
        *outImage = VK_NULL_HANDLE;
        *outMemory = VK_NULL_HANDLE;
        return;
    }

    vkBindImageMemory(device, *outImage, *outMemory, 0);

    if (outView) {
        VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = *outImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = fmt;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, outView) != VK_SUCCESS) {
            std::fprintf(stderr, "createOrResizeImage: vkCreateImageView failed\n");
            *outView = VK_NULL_HANDLE;
        }
    }
}

VkCommandBuffer beginSingleUseCommands(VkDevice device, VkCommandPool pool)
{
    VkCommandBufferAllocateInfo alloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandPool = pool;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult r = vkAllocateCommandBuffers(device, &alloc, &cmd);
    if (r != VK_SUCCESS || cmd == VK_NULL_HANDLE) {
        std::fprintf(stderr, "beginSingleUseCommands: vkAllocateCommandBuffers failed (code=%d)\n", (int)r);
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    r = vkBeginCommandBuffer(cmd, &begin);
    if (r != VK_SUCCESS) {
        std::fprintf(stderr, "beginSingleUseCommands: vkBeginCommandBuffer failed (code=%d)\n", (int)r);
        // Free the cmd we allocated
        vkFreeCommandBuffers(device, pool, 1, &cmd);
        return VK_NULL_HANDLE;
    }
    return cmd;
}

void endSingleUseCommands(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer cmd)
{
    if (cmd == VK_NULL_HANDLE) return;

    VkResult r = vkEndCommandBuffer(cmd);
    if (r != VK_SUCCESS) {
        std::fprintf(stderr, "endSingleUseCommands: vkEndCommandBuffer failed (code=%d)\n", (int)r);
        // attempt to free command buffer anyway
        vkFreeCommandBuffers(device, pool, 1, &cmd);
        return;
    }

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    r = vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
    if (r != VK_SUCCESS) {
        std::fprintf(stderr, "endSingleUseCommands: vkQueueSubmit failed (code=%d)\n", (int)r);
    } else {
        vkQueueWaitIdle(queue);
    }

    vkFreeCommandBuffers(device, pool, 1, &cmd);
}

void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat fmt, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        // Generic fallback
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image, uint32_t w, uint32_t h)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { w, h, 1 };

    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat fmt)
{
    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = fmt;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
        std::fprintf(stderr, "createImageView: vkCreateImageView failed\n");
        return VK_NULL_HANDLE;
    }
    return view;
}

static VkSampler s_sampler = VK_NULL_HANDLE;

VkSampler GetOrCreateSampler(VkDevice device)
{
    if (s_sampler != VK_NULL_HANDLE) return s_sampler;

    VkSamplerCreateInfo samp{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samp.magFilter = VK_FILTER_LINEAR;
    samp.minFilter = VK_FILTER_LINEAR;
    samp.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samp.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samp.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samp.anisotropyEnable = VK_FALSE;
    samp.maxAnisotropy = 1.0f;
    samp.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samp.unnormalizedCoordinates = VK_FALSE;
    samp.compareEnable = VK_FALSE;
    samp.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samp.mipLodBias = 0.0f;
    samp.minLod = 0.0f;
    samp.maxLod = 0.0f;

    if (vkCreateSampler(device, &samp, nullptr, &s_sampler) != VK_SUCCESS) {
        std::fprintf(stderr, "getOrCreateSampler: vkCreateSampler failed\n");
        return VK_NULL_HANDLE;
    }
    return s_sampler;
}

void DestroyVulkanHelpersResources(VkDevice device)
{
    // Ensure device isn't executing commands that reference these objects.
    vkDeviceWaitIdle(device);

    if (s_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, s_sampler, nullptr);
        s_sampler = VK_NULL_HANDLE;
    }
}

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Check if this memory type bit is set in the typeFilter
        bool isSuitableType = (typeFilter & (1 << i));

        // Check if this memory type has all the required property flags
        bool hasRequiredProperties = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;

        if (isSuitableType && hasRequiredProperties) {
            return i;
        }
    }

    // If we get here, something is wrong (e.g., asking for a type the GPU doesn't have)
    throw std::runtime_error("Failed to find suitable memory type!");
}

void TransitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleUseCommands(device, pool);

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    // Define pipeline stages based on layouts
    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    // Add other transitions (like TRANSFER_DST_OPTIMAL) as needed

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleUseCommands(device, queue, pool, commandBuffer);
}

void TransitionImageLayoutInline(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } // ... add other cases as needed

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

std::vector<int8> LoadShaderCode(const std::string& filename)
{
    FILE* file = std::fopen(filename.c_str(), "rb");
    if (!file) {
        std::fprintf(stderr, "Failed to open shader file: %s\n", filename.c_str());
        return {};
    }

    std::fseek(file, 0, SEEK_END);
    long size = std::ftell(file);
    std::rewind(file);

    if (size <= 0) {
        std::fprintf(stderr, "Shader file is empty or error occurred: %s\n", filename.c_str());
        std::fclose(file);
        return {};
    }

    std::vector<int8> code(size);
    size_t readSize = std::fread(code.data(), 1, size, file);
    std::fclose(file);

    if (readSize != static_cast<size_t>(size)) {
        std::fprintf(stderr, "Failed to read entire shader file: %s\n", filename.c_str());
        return {};
    }

    return code;
}

VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<int8>& code) {
    // Vulkan requires codeSize to be a multiple of 4 and pCode to be aligned to 4 bytes.
    size_t byteSize = code.size();
    if (byteSize == 0) {
        std::fprintf(stderr, "CreateShaderModule: empty shader code\n");
        return VK_NULL_HANDLE;
    }

    size_t wordCount = (byteSize + 3) / 4;
    std::vector<uint32_t> words(wordCount);
    // Copy bytes into words (preserve little-endian byte order)
    std::memset(words.data(), 0, wordCount * sizeof(uint32_t));
    std::memcpy(words.data(), code.data(), byteSize);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = wordCount * sizeof(uint32_t);
    createInfo.pCode = words.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::fprintf(stderr, "CreateShaderModule: vkCreateShaderModule failed\n");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

void* CreateAndMapBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // 1. Specify buffer creation info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    // Buffer used as a vertex buffer and data transfer source
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

    // 2. Query memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // 3. Find a suitable memory type index that is host visible
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t memoryTypeIndex = ~0U;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == ~0U) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "Failed to find suitable memory type");
        return nullptr;
    }

    // 4. Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);

    // 5. Bind the allocated memory to the buffer
    vkBindBufferMemory(device, buffer, bufferMemory, 0);

    // 6. Map the memory and copy data
    void* data;
    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &data);

    return data;

    // Example: copy some data to the mapped memory (e.g., vertex data)
    // std::vector<float> vertexData = { /* ... your vertex data ... */ };
    // memcpy(data, vertexData.data(), (size_t) bufferSize);

    // If VK_MEMORY_PROPERTY_HOST_COHERENT_BIT was NOT used,
    // you would need to call vkFlushMappedMemoryRanges here.
    // vkFlushMappedMemoryRanges(device, 1, &mappedRange);

    // Unmapping is not strictly necessary if you keep the buffer mapped for its lifetime (persistent mapping)
    // which can be more efficient.
    // If you do unmap:
    // vkUnmapMemory(device, bufferMemory);
}

bool MapVulkanMemoryLocation(const uint32 memory_location, const VkBuffer& buffer, const VkDevice& device, const VkDescriptorSet& descriptorSet)
{
    if (buffer == VK_NULL_HANDLE) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "MapVulkanMemoryLocation: buffer is VK_NULL_HANDLE");
        return false;
    }

    if (device == VK_NULL_HANDLE) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "MapVulkanMemoryLocation: device is VK_NULL_HANDLE");
        return false;
    }

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = memory_location;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.descriptorCount = 1;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet  descriptorWrite{};
    descriptorWrite.dstBinding = memory_location;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    return true;
}

VulkanResource CreateMappedStorageBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize& size, const uint32_t bindingPoint)
{
    VulkanResource res{};

    // 1. Create Buffer (MUST include STORAGE_BUFFER_BIT)
    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &bufferInfo, nullptr, &res.buffer);

    // 2. Allocate & Bind Memory (Host Coherent)
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, res.buffer, &memReqs);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    uint32_t memType = ~0U;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memReqs.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memType = i; break;
        }
    }

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memType;
    vkAllocateMemory(device, &allocInfo, nullptr, &res.memory);
    vkBindBufferMemory(device, res.buffer, res.memory, 0);
    vkMapMemory(device, res.memory, 0, size, 0, &res.mappedData);

    // 3. Create Descriptor Set Layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = bindingPoint;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL; // Visible to all stages

    VkDescriptorSetLayoutCreateInfo layoutCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutCI.bindingCount = 1;
    layoutCI.pBindings = &layoutBinding;
    vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &res.layout);

    // 4. Create Descriptor Pool
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
    VkDescriptorPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes = &poolSize;
    poolCI.maxSets = 1;
    vkCreateDescriptorPool(device, &poolCI, nullptr, &res.pool);

    // 5. Allocate and Update Descriptor Set
    VkDescriptorSetAllocateInfo setAlloc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    setAlloc.descriptorPool = res.pool;
    setAlloc.descriptorSetCount = 1;
    setAlloc.pSetLayouts = &res.layout;
    vkAllocateDescriptorSets(device, &setAlloc, &res.descriptorSet);

    VkDescriptorBufferInfo bInfo{ res.buffer, 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = res.descriptorSet;
    write.dstBinding = bindingPoint;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    return res;
}

void BindPipelineToBuffer(const VkCommandBuffer& cmd, const VkPipelineLayout& pipeline_layout, const VulkanResource& resource)
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &resource.descriptorSet, 0, nullptr);
}