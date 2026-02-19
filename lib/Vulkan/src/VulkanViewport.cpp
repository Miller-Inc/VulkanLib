//
// Created by James Miller on 2/10/2026.
//

// Rewritten VulkanViewport to use a host-mapped SSBO updated by the CPU each frame.
// Key points:
// - SSBO (storage buffer) is HOST_VISIBLE | HOST_COHERENT so CPU can write vertices directly.
// - Vertex shader reads positions[] from set=0 binding=0 using gl_VertexIndex; no vertex buffers required.
// - Pipeline layout includes the descriptor set layout and push constants.
// - Cleanup now actively destroys/free/unmaps Vulkan resources and waits on device.

#include "VulkanViewport.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "Logger.h"
#include "OperatorsExtentions.h"
#include "stb_image_write.h" // no implementation macro here
#include "VulkanHelpers.h"
#include <vector>
#include "GInstance.h"
#include "VertexInfo.h"

VulkanViewport::VulkanViewport(void* gInstance)
{
    mInstance = (GInstance*)gInstance;
    ImageResource = VkImageResource(gInstance);

    // Assign a rough debug id (pointer truncated)
    mId = (uint64_t)(uintptr_t)this;

    // Initialize handles to known safe values
    mPipeline = VK_NULL_HANDLE;
    PipelineLayout = VK_NULL_HANDLE;
    DescriptorSetLayout = VK_NULL_HANDLE;
    DescriptorPool = VK_NULL_HANDLE;
    DescriptorSet = VK_NULL_HANDLE;
    SSBOBuffer = VK_NULL_HANDLE;
    SSBOBufferMemory = VK_NULL_HANDLE;
    SSBOMappedPtr = nullptr;
    SSBOSize = 0;
}

VulkanViewport::VulkanViewport(void* gInstance, const int32 Width, const int32 Height, const uint8 Type)
{
    mInstance = (GInstance*)gInstance;
    this->Width = Width;
    this->Height = Height;
    ImageResource = VkImageResource(gInstance);
    ImageResource.Width = Width;
    ImageResource.Height = Height;

    // Initialize handles
    mPipeline = VK_NULL_HANDLE;
    PipelineLayout = VK_NULL_HANDLE;
    DescriptorSetLayout = VK_NULL_HANDLE;
    DescriptorPool = VK_NULL_HANDLE;
    DescriptorSet = VK_NULL_HANDLE;
    SSBOBuffer = VK_NULL_HANDLE;
    SSBOBufferMemory = VK_NULL_HANDLE;
    SSBOMappedPtr = nullptr;
    SSBOSize = 0;

    if (Type == VIEWPORT_NO_SHADERS)
    {
        GenerateCanvas();
    }
    else
    {
        GenerateRenderTarget();
    }
}

void VulkanViewport::GenerateCanvas()
{
    ImageResource = VkImageResource(mInstance);
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    ImageResource.CreateCanvas(Width, Height, Colors::RandomColor());
    const int num = rand() % 20 + 5;
    for (int i = 0; i < num; i++)
    {
        if (i % 15 == 0)
        {
            ImageResource.DrawFilledTriangle(rand() % Width, rand() % Height,
                rand() % Width, rand() % Height,
                rand() % Width, rand() % Height,
                Colors::RandomColor());
        }
        else if (i % 10 == 0)
        {
            ImageResource.DrawLine(rand() % Width, rand() % Height, rand() % Width, rand() % Height, Colors::RandomColor(), rand() % 7 + 1);
        }

        if (rand() % 16 == 0)
            ImageResource.DrawRect(rand() % Width, rand() % Height, rand() % 200, rand() % 150, Colors::RandomColor());
        else
            ImageResource.DrawFilledCircle(rand() % Width, rand() % Height, rand() % 125, Colors::RandomColor());

    }
    ImageResource.DumpToPNG("dump.png");
}

void VulkanViewport::GenerateRenderTarget()
{
    if (bInit && Type == VIEWPORT_TEST_SHADERS)
    {
        return;
    }

    // Do not mark bInit true until we successfully create all resources below.
    Type = VIEWPORT_TEST_SHADERS;

    // Validate instance and Vulkan setup
    if (!mInstance) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "VulkanViewport::GenerateRenderTarget: mInstance is null");
        return;
    }
    const GPU::VulkanSetup* setup = mInstance->GetVulkanSetup();
    if (!setup || setup->device == VK_NULL_HANDLE) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "VulkanViewport::GenerateRenderTarget: invalid Vulkan setup or device");
        return;
    }

    VkDevice device = setup->device;
    VkPhysicalDevice phys = setup->physicalDevice;

    ImageResource = VkImageResource(mInstance);

    // Ensure we have sensible dimensions before creating the render target.
    // If Width/Height are zero, fallback to ImageResource's existing size or a default 256.
    int desiredW = (Width > 0) ? Width : (ImageResource.Width > 0 ? (int)ImageResource.Width : 256);
    int desiredH = (Height > 0) ? Height : (ImageResource.Height > 0 ? (int)ImageResource.Height : 256);
    this->Width = desiredW;
    this->Height = desiredH;
    ImageResource.Width = (uint32_t)desiredW;
    ImageResource.Height = (uint32_t)desiredH;

    ImageResource.CreateRenderTarget(this->Width, this->Height);

    // Validate that the image resource created a render pass and framebuffer
    if (ImageResource.RenderPass == VK_NULL_HANDLE || ImageResource.Framebuffer == VK_NULL_HANDLE) {
        M_LOGGER(Logger::LogGraphics, Logger::Error, "VulkanViewport::GenerateRenderTarget - ImageResource.CreateRenderTarget failed to create RenderPass/Framebuffer");
        return;
    }

    // Mark that this viewport's image was created successfully.
    mImageCreated = true;

    // Cache handles locally in case ImageResource object gets replaced elsewhere.
    mRenderPass = ImageResource.RenderPass;
    mFramebuffer = ImageResource.Framebuffer;
    mImageWidth = ImageResource.Width;
    mImageHeight = ImageResource.Height;

    // If we are re-creating resources, destroy previous ones safely first
    vkDeviceWaitIdle(device);

    if (mPipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, mPipeline, nullptr); mPipeline = VK_NULL_HANDLE; }
    if (PipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, PipelineLayout, nullptr); PipelineLayout = VK_NULL_HANDLE; }
    if (DescriptorPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, DescriptorPool, nullptr); DescriptorPool = VK_NULL_HANDLE; }
    if (DescriptorSetLayout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, DescriptorSetLayout, nullptr); DescriptorSetLayout = VK_NULL_HANDLE; }
    if (SSBOMappedPtr && SSBOBufferMemory != VK_NULL_HANDLE) { vkUnmapMemory(device, SSBOBufferMemory); SSBOMappedPtr = nullptr; }
    if (SSBOBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, SSBOBuffer, nullptr); SSBOBuffer = VK_NULL_HANDLE; }
    if (SSBOBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device, SSBOBufferMemory, nullptr); SSBOBufferMemory = VK_NULL_HANDLE; SSBOSize = 0; }

    // Load shader modules
    const auto vert = LoadShaderCode(std::string(SHADER_DIR) + "/shader.vert.spv");
    const auto frag = LoadShaderCode(std::string(SHADER_DIR) + "/shader.frag.spv");

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    if (!vert.empty()) vertModule = CreateShaderModule(device, vert);
    if (!frag.empty()) fragModule = CreateShaderModule(device, frag);

    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        if (vertModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, vertModule, nullptr);
        if (fragModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, fragModule, nullptr);
        M_LOGGER(Logger::LogGraphics, Logger::Error, "VulkanViewport::GenerateRenderTarget - failed to load shader modules (vert=%p frag=%p)", (void*)vertModule, (void*)fragModule);
        return;
    }

    VkPipelineShaderStageCreateInfo vertShaderInfo{};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderInfo.module = vertModule;
    vertShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderInfo{};
    fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderInfo.module = fragModule;
    fragShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderInfo, fragShaderInfo };

    // Vertex input: EMPTY because vertex positions come from SSBO and shader uses gl_VertexIndex.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport state (dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic states: viewport & scissor
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(2);
    dynamicState.pDynamicStates = dynamicStates;

    // Descriptor set layout for SSBO (set=0, binding=0)
    // VkDescriptorSetLayoutBinding ssboBinding{};
    // ssboBinding.binding = 0;
    // ssboBinding.descriptorCount = 1;
    // ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    // ssboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // ssboBinding.pImmutableSamplers = nullptr;
    //
    // VkDescriptorSetLayoutCreateInfo dsLayoutInfo{};
    // dsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // dsLayoutInfo.bindingCount = 2;
    // dsLayoutInfo.pBindings = &ssboBinding;
    //
    // if (vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &DescriptorSetLayout) != VK_SUCCESS) {
    //     if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
    //     if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);
    //     throw std::runtime_error("Failed to create descriptor set layout for SSBO");
    // }

    // Push constants
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MeshPushConstants);

    // Pipeline layout including the descriptor set layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, DescriptorSetLayout, nullptr);
        DescriptorSetLayout = VK_NULL_HANDLE;
        if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = PipelineLayout;
    pipelineInfo.renderPass = ImageResource.RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult createRes = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline);

    // Shader modules can be destroyed once pipeline is created
    if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
    if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);

    if (createRes != VK_SUCCESS) {
        // Provide a detailed log to help diagnose runtime errors when binding/using the pipeline
        M_LOGGER(Logger::LogGraphics, Logger::Error, "VulkanViewport::GenerateRenderTarget: vkCreateGraphicsPipelines failed (code=%d)", (int)createRes);
        vkDestroyPipelineLayout(device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(device, DescriptorSetLayout, nullptr);
        DescriptorSetLayout = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // ---- Create SSBO and descriptor set, upload small triangle data ----
    std::vector<Vec4> triangle = {
        { 0.0f, -0.5f, 0.0f, 1.0f },
        { 0.5f,  0.5f, 0.0f, 1.0f },
        {-0.5f,  0.5f, 0.0f, 1.0f }
    };

    VkDeviceSize bufferSize = sizeof(Vec4) * triangle.size();
    bufferSize = sizeof(VertexInfo) * 3000; // space for 1000 triangles (3000 vertices)

    VertResource = CreateMappedStorageBuffer(device, phys, bufferSize, 0);
    // Save convenience aliases used elsewhere in the class (SSBOMappedPtr/SSBOSize)
    SSBOMappedPtr = VertResource.mappedData;
    SSBOSize = bufferSize;

    bufferSize = sizeof(uint32) * 3 * 1000; // space for 1000 triangles (3000 vertices)
    TriResource = CreateMappedStorageBuffer(device, phys, bufferSize, 1);
    TriangleBufferPtr = TriResource.mappedData;



    // Everything succeeded - mark initialized
    bInit = true;
}

void VulkanViewport::Render()
{
    if (!mInstance) return;
    // Debug: show ImageResource object pointer and its current state so we can tell if it changes between creation and render.
    const GPU::VulkanSetup* setup = mInstance->GetVulkanSetup();
    if (!setup || setup->device == VK_NULL_HANDLE) return;
    if (setup->commandPool == VK_NULL_HANDLE || setup->queue == VK_NULL_HANDLE) {
        std::cerr << "VulkanViewport::Render - invalid commandPool or queue in VulkanSetup" << std::endl;
        return;
    }
    VkDevice device = setup->device;

    // Do not attempt heavy resource creation here. Render should be a fast path.
    // If resources are not ready, just skip rendering this frame and let the application
    // trigger GenerateRenderTarget() explicitly (for example on resize or initialization).
    if (ImageResource.RenderPass == VK_NULL_HANDLE || ImageResource.Framebuffer == VK_NULL_HANDLE || ImageResource.Width == 0 || ImageResource.Height == 0) {
        std::cerr << "VulkanViewport::Render - ImageResource not ready (renderpass/framebuffer missing); skipping frame." << std::endl;
        return;
    }

    if (mPipeline == VK_NULL_HANDLE || PipelineLayout == VK_NULL_HANDLE) {
        std::cerr << "VulkanViewport::Render - pipeline or pipeline layout missing; skipping frame." << std::endl;
        return;
    }

    // Check the combined descriptor set we created for both buffers.
    // if (DescriptorSet == VK_NULL_HANDLE) {
    //     std::cerr << "VulkanViewport::Render - descriptor set missing; skipping frame." << std::endl;
    //     return;
    // }

    // Update SSBO from CPU (rotate/translate based on EngineTime)
    if (VertResource.mappedData != nullptr) {
        const float t = ((GInstance*)mInstance)->EngineTime;
        float s = sinf(t);
        float c = cosf(t);

        VertexInfo base[3] = {
            VertexInfo(-0.25f, -0.25f, 0.0f, 0.0f, 0.0f),
            VertexInfo(0.25f, -0.25f, 0.0f, 1.0f, 0.0f),
            VertexInfo(0.0f, 0.25f, 0.0f, 0.5f, 1.0f)
        };

        VertexInfo out[3];
        for (int i = 0; i < 3; ++i) {
            out[i].x = c * base[i].x - s * base[i].y;
            out[i].y = s * base[i].x + c * base[i].y;
            out[i].z = base[i].z;
            out[i].u = base[i].u;
            out[i].v = base[i].v;
        }
        memcpy(VertResource.mappedData, out, sizeof(out));
        // If memory were not HOST_COHERENT we'd call vkFlushMappedMemoryRanges here
    }

    const VkCommandBuffer cmd = beginSingleUseCommands(setup->device, setup->commandPool);
    if (cmd == VK_NULL_HANDLE) {
        std::cerr << "VulkanViewport::Render - beginSingleUseCommands returned VK_NULL_HANDLE" << std::endl;
        return;
    }

    // Prepare push constants
    MeshPushConstants constants{};
    constants.time = ((GInstance*)mInstance)->EngineTime;
    constants.r = (sinf(constants.time) + 1.0f) * 0.5f;
    constants.g = (cosf(constants.time) + 1.0f) * 0.5f;
    constants.b = (sinf(constants.time * 0.5f) + 1.0f) * 0.5f;
    float3 triangleVerts[3] = {
        {-0.5f * cosf(constants.time) - 0.5f, -0.5f * sinf(constants.time), 1.0f},
        {0.5f * cosf(constants.time) - 0.25f, 0.5f * sinf(constants.time) - 0.25f, 1.0f},
        {-0.5f * cosf(constants.time) + 0.25f,  0.5f * sinf(constants.time) + 0.25f, 1.0f}
    };

    float3 triangleNormals[3] = {
        {1.0f, 0.0f, 0.765f},
        {0.267f, 0.871f, 0.94f},
        {0.184f, 1.0f, 0.0f}
    };

    memcpy(constants.positions, triangleVerts, sizeof(triangleVerts));
    memcpy(constants.normals, triangleNormals, sizeof(triangleNormals));

    // Draw: ImageResource::DrawWithShaders will begin the renderpass and bind pipeline, push constants, and descriptor sets.
    // Ensure the ImageResource has a valid renderpass/framebuffer before attempting to render into it.
    if (ImageResource.Width > 0 || ImageResource.Height > 0) {
        // If we have cached handles from a previous successful GenerateRenderTarget, use them.
        if (mRenderPass != VK_NULL_HANDLE && mFramebuffer != VK_NULL_HANDLE && mImageWidth > 0 && mImageHeight > 0) {

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            VkRenderPassBeginInfo rpBegin{};
            rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBegin.renderPass = mRenderPass;
            rpBegin.framebuffer = mFramebuffer;
            rpBegin.renderArea.offset = {0,0};
            rpBegin.renderArea.extent = { mImageWidth, mImageHeight };
            rpBegin.clearValueCount = 1;
            rpBegin.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

            // Bind pipeline/layout
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

            // Set viewport/scissor
            VkViewport vp{0.0f, 0.0f, (float)mImageWidth, (float)mImageHeight, 0.0f, 1.0f};
            vkCmdSetViewport(cmd, 0, 1, &vp);
            VkRect2D scissor{{0,0},{mImageWidth, mImageHeight}};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // Push constants
            if (PipelineLayout != VK_NULL_HANDLE) {
            vkCmdPushConstants(cmd, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants), &constants);
            }

            // Bind descriptor set (SSBO) if present
            // if (DescriptorSet != VK_NULL_HANDLE && PipelineLayout != VK_NULL_HANDLE) {
                // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);
            // }


            // std::vector<VkDescriptorSet> dsPtr = { VertResource.descriptorSet, TriResource.descriptorSet, ImageResource.DescriptorSet };
            // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, static_cast<uint32_t>(dsPtr.size()), dsPtr.data(), 0, nullptr);

            BindPipelineToBuffer(cmd, PipelineLayout, VertResource);
            BindPipelineToBuffer(cmd, PipelineLayout, TriResource);

            // Draw the triangle (3 vertices uploaded)
            vkCmdDraw(cmd, 3, 1, 0, 0);

            vkCmdEndRenderPass(cmd);

            // Finish command buffer and return normally (endSingleUseCommands will be called below)
        } else {
            std::cerr << "VulkanViewport::Render - ImageResource still not ready (RenderPass/Framebuffer missing) after regeneration attempt; skipping DrawWithShaders." << std::endl;
            // End and free the command buffer we began.
            endSingleUseCommands(setup->device, setup->queue, setup->commandPool, cmd);
            return;
        }
    }

    // ImageResource.DrawWithShaders(cmd, mPipeline, PipelineLayout, DescriptorSet, &constants);

    // If the ImageResource itself has valid handles, prefer its DrawWithShaders implementation.
    // if (ImageResource.RenderPass != VK_NULL_HANDLE && ImageResource.Framebuffer != VK_NULL_HANDLE) {
    //     std::vector<VkDescriptorSet> dsPtr = { VertResource.descriptorSet, TriResource.descriptorSet, ImageResource.DescriptorSet };
    //
    //     ImageResource.DrawWithShaders(cmd, mPipeline, PipelineLayout, 6, dsPtr.data(), dsPtr.size(), &constants);
    // } else {
    //     std::cerr << "VulkanViewport::Render - used cached renderpass/framebuffer to draw" << std::endl;
    // }

    endSingleUseCommands(setup->device, setup->queue, setup->commandPool, cmd);
}

VulkanViewport::~VulkanViewport()
{
    Cleanup();
}

void VulkanViewport::Cleanup()
{
    if (!mInstance || true) return;
    const GPU::VulkanSetup* setup = nullptr;
    try { setup = mInstance->GetVulkanSetup(); } catch (...) { setup = nullptr; }
    if (!setup) return;

    VkDevice device = setup->device;
    if (device == VK_NULL_HANDLE) return;

    // Wait for the device to be idle before destroying resources
    vkDeviceWaitIdle(device);

    // Destroy ImGui texture / render target resources
    ImageResource.DestroyTexture(mInstance);

    // Destroy helper-owned static resources
    DestroyVulkanHelpersResources(device);

    if (mPipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, mPipeline, nullptr); mPipeline = VK_NULL_HANDLE; }
    if (PipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, PipelineLayout, nullptr); PipelineLayout = VK_NULL_HANDLE; }
    if (DescriptorPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, DescriptorPool, nullptr); DescriptorPool = VK_NULL_HANDLE; }
    if (DescriptorSetLayout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, DescriptorSetLayout, nullptr); DescriptorSetLayout = VK_NULL_HANDLE; }

    if (SSBOMappedPtr && SSBOBufferMemory != VK_NULL_HANDLE) {
        vkUnmapMemory(device, SSBOBufferMemory);
        SSBOMappedPtr = nullptr;
    }
    if (SSBOBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, SSBOBuffer, nullptr); SSBOBuffer = VK_NULL_HANDLE; }
    if (SSBOBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device, SSBOBufferMemory, nullptr); SSBOBufferMemory = VK_NULL_HANDLE; }
    SSBOSize = 0;

    if (VertResource.layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, VertResource.layout, nullptr); VertResource.layout = VK_NULL_HANDLE; }
    if (VertResource.pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, VertResource.pool, nullptr); VertResource.pool = VK_NULL_HANDLE; }
    if (TriResource.layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, TriResource.layout, nullptr); TriResource.layout = VK_NULL_HANDLE; }
    if (TriResource.pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, TriResource.pool, nullptr); TriResource.pool = VK_NULL_HANDLE; }


    // Everything succeeded - mark initialized
    bInit = false;
}

VkDescriptorSet VulkanViewport::GetDescriptorSet() const
{
    return ImageResource.DescriptorSet;
}

void VulkanViewport::UpdateGInstance(GInstance* newInstance)
{
    mInstance = newInstance;
    ImageResource = VkImageResource(newInstance);
}

void VulkanViewport::SetTrianglePositions(const float3* verts, const size_t count) const
{
    // Expect up to 3 vertices; if not available, do nothing.
    // if (!SSBOMappedPtr || SSBOSize < sizeof(float3) * 3) return;
    if (count < 3) return;
    // size_t toCopy = (count > 3) ? 3 : count;
    // memcpy(SSBOMappedPtr, verts, sizeof(float3) * toCopy);

    memcpy(VertResource.mappedData, verts, count * sizeof(float3));
}

void VulkanViewport::SetTriangleIndices(const uint32* indices, const size_t count) const
{
    if (count < 3) return;
    memcpy(TriResource.mappedData, indices, count * sizeof(uint32));
}

void VulkanViewport::SetVertexInfo(const VertexInfo* vertexInfo, const size_t count) const
{
    if (count < 3) return;
    memcpy(VertResource.mappedData, vertexInfo, count * sizeof(VertexInfo));
}


