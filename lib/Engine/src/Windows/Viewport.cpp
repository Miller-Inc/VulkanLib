//
// Created by James Miller on 11/29/2025.
//

#include "Windows/Viewport.h"
#include "Windows/WindowBase.h"
#include "Logger.h"
#include "imgui_impl_vulkan.h"
#include "VulkanHelpers.h"
#include <vector>
#include <SDL3/SDL.h>
#include "GInstance.h" // Forward declaration of Instance
#include "BaseTypes/Image.h"
#include <chrono>
#include "OperatorsExtentions.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // no implementation macro here
#define RNG_CANVAS false

int WIDTH = 1300;
int HEIGHT = 780;

void Viewport::GenerateCanvas()
{
    // mImage = Image(mInstance);
    // srand(std::chrono::system_clock::now().time_since_epoch().count());
    // mImage.ImageResource.CreateCanvas(WIDTH, HEIGHT, Colors::RandomColor());
    // const int num = rand() % 20 + 5;
    // for (int i = 0; i < num; i++)
    // {
    //     if (i % 15 == 0)
    //     {
    //         mImage.ImageResource.DrawFilledTriangle(rand() % WIDTH, rand() % HEIGHT,
    //             rand() % WIDTH, rand() % HEIGHT,
    //             rand() % WIDTH, rand() % HEIGHT,
    //             Colors::RandomColor());
    //     }
    //     else if (i % 10 == 0)
    //     {
    //         mImage.ImageResource.DrawLine(rand() % WIDTH, rand() % HEIGHT, rand() % WIDTH, rand() % HEIGHT, Colors::RandomColor(), rand() % 7 + 1);
    //     }
    //
    //     if (rand() % 16 == 0)
    //         mImage.ImageResource.DrawRect(rand() % WIDTH, rand() % HEIGHT, rand() % 200, rand() % 150, Colors::RandomColor());
    //     else
    //         mImage.ImageResource.DrawFilledCircle(rand() % WIDTH, rand() % HEIGHT, rand() % 125, Colors::RandomColor());
    //
    // }
    // mImage.ImageResource.DumpToPNG("dump.png");
    mViewport.GenerateCanvas();
}

void Viewport::GenerateCleanCanvas()
{
    // mImage = Image(mInstance);
    // mImage.ImageResource.CreateRenderTarget(WIDTH, HEIGHT);
    // VkDevice device = mInstance->GetVulkanSetup()->device;
    // VkPhysicalDevice phys = mInstance->GetVulkanSetup()->physicalDevice;
    //
    // // Create simple graphics pipeline
    // const auto vert = LoadShaderCode(std::string(SHADER_DIR)/"shader.vert.spv");
    // auto module = CreateShaderModule(mInstance->GetVulkanSetup()->device, vert);
    // VkPipelineShaderStageCreateInfo vertShaderInfo{};
    // vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // vertShaderInfo.module = module;
    // vertShaderInfo.pName = "main";
    //
    // const auto frag = LoadShaderCode(std::string(SHADER_DIR)/"shader.frag.spv");
    // const auto frag_module = CreateShaderModule(mInstance->GetVulkanSetup()->device, frag);
    // VkPipelineShaderStageCreateInfo fragShaderInfo{};
    // fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // fragShaderInfo.module = frag_module;
    // fragShaderInfo.pName = "main";
    //
    // VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderInfo, fragShaderInfo };

    mViewport.GenerateRenderTarget();

}

void Viewport::TestRender()
{
    mViewport.Render();
}

Viewport::Viewport()
{
    Name = "Viewport";
    isOpen = false;
    mImage = Image(nullptr);
    // mViewport default-constructed; update its GInstance when Init() is called
}

Viewport::Viewport(const Viewport& other)
 : WindowBase(other) {
    Name = other.Name;
    isOpen = false; // Always start closed
    mInstance = other.mInstance; // Copy the instance pointer
    mImage = Image(mInstance); // Create a new Image with the same instance
    // Reuse existing mViewport and update its instance to avoid copying Vulkan handles
    mViewport.UpdateGInstance(mInstance);
}

void Viewport::Init()
{
    Viewport::Init("Viewport", nullptr);
}

void Viewport::Init(const std::string& WindowName, GInstance* Instance)
{
    WindowBase::Init(WindowName, Instance);

    mImage = Image(mInstance);
    mViewport = {mInstance, WIDTH, HEIGHT, VIEWPORT_TEST_SHADERS};

    // Load a canvas to display
#if RNG_CANVAS
     GenerateCanvas();
#else
    // mViewport.UpdateGInstance(mInstance);
    GenerateCleanCanvas();
    TestRender();
#endif
}

void Viewport::Init(GInstance* GameInstance)
{
    Init("Viewport", GameInstance);
}

void Viewport::Open()
{
    WindowBase::Open();
}

void Viewport::Draw(const float deltaTime)
{
    WindowBase::Draw(deltaTime);

    mViewport.Render();

    if (avg_tick_time == 0.0f)
        avg_tick_time = deltaTime;
    else
        avg_tick_time = avg_tick_time * 0.95f + deltaTime * 0.05f;

    if (ImGui::Begin("Editor", &bEditorOpen))
    {
        if (ImGui::Button("Regenerate Canvas"))
        {
            #if RNG_CANVAS
            GenerateCanvas();
            #endif
        }
        ImGui::InputInt("Width###Width_Input", &WIDTH);
        ImGui::InputInt("Height###HEIGHT_Input", &HEIGHT);
        if (ImGui::Button("Request Quit###Quit_Button"))
        {
            if (mInstance)
            {
                mInstance->StopMainLoop();
            }
        }
    }
    ImGui::End();


    static bool window = true;
    ImGui::SetNextWindowSize(ImVec2(1320, 800));

    if (ImGui::Begin("Viewport", &window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {

        ImGui::Image(
            mViewport.ImageResource.GetID(),
            ImVec2((float)mViewport.ImageResource.Width, (float)mViewport.ImageResource.Height),
            ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // tint: use white to display texture unchanged
            ImVec4(0.0f, 0.0f, 0.0f, 0.0f)  // border: transparent (or set to a color you want)
        );
    }
    ImGui::End();
}

void Viewport::Close()
{
    // Ensure mouse capture is released if still active
    if (mMouseCaptured) {
        mMouseCaptured = false;
        // SDL_SetRelativeMouseMode(false);
        SDL_ShowCursor();
    }

    // Existing cleanup that destroys Vulkan resources and ImGui texture
    mViewport.Cleanup();

    bEditorOpen = false;

    M_LOGGER(Logger::LogGraphics, Logger::Info, "Average tick time = %f", avg_tick_time);

    WindowBase::Close();
}

void Viewport::Tick(const float deltaTime)
{
    // Read keyboard state using SDL
    auto kb = SDL_GetKeyboardState(nullptr);
    const uint8_t* keyboard = reinterpret_cast<const uint8_t*>(kb);

    float move = mMoveSpeed * deltaTime;
    if (keyboard[SDL_SCANCODE_LEFT]) {
        mTriX -= move;
    }
    if (keyboard[SDL_SCANCODE_RIGHT]) {
        mTriX += move;
    }
    if (keyboard[SDL_SCANCODE_UP]) {
        mTriY += move;
    }
    if (keyboard[SDL_SCANCODE_DOWN]) {
        mTriY -= move;
    }

    // Clamp to [-1, 1] in NDC
    mTriX = std::clamp(mTriX, -1.0f, 1.0f);
    mTriY = std::clamp(mTriY, -1.0f, 1.0f);

    // Prepare new triangle positions in NDC (Vec4)
    // float3 verts[3];
    // verts[0] = { -0.2f + mTriX, -0.5f + mTriY, 0.0f };
    // verts[1] = {  0.5f + mTriX,  0.5f + mTriY, 0.0f };
    // verts[2] = { -0.5f + mTriX,  0.5f + mTriY, 0.0f };

    // Push into the viewport SSBO (no-op if not initialized)
    // mViewport.SetTrianglePositions(verts, 3);
}
