//
// Created by James Miller on 11/29/2025.
//

#include "Windows/Viewport.h"
#include "Windows/WindowBase.h"
#include "imgui_impl_vulkan.h"
#include "VulkanHelpers.h"
#include <vector>
#include <SDL3/SDL.h>
#include "GInstance.h" // Forward declaration of Instance
#include "BaseTypes/Image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <chrono>

#include "stb_image_write.h" // no implementation macro here

static bool SavePNG(const std::string& path, const std::vector<uint8_t>& pixels, unsigned int w, unsigned int h, unsigned int c)
{
    if (pixels.empty() || w == 0 || h == 0 || (c < 1 || c > 4)) return false;
    // stride in bytes per row
    int stride_bytes = int(w * c);
    // stbi_write_png returns non-zero on success
    return stbi_write_png(path.c_str(), int(w), int(h), int(c), pixels.data(), stride_bytes) != 0;
}

void Viewport::GenerateCanvas()
{
    mImage = Image(mInstance);
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    mImage.ImageResource.CreateCanvas(800, 600, Colors::RandomColor());
    int num = rand() % 20 + 5;
    for (int i = 0; i < num; i++)
    {
        if (i % 10 == 0)
        {
            mImage.ImageResource.DrawLine(rand() % 800, rand() % 600, rand() % 800, rand() % 600, Colors::RandomColor(), rand() % 7 + 1);
        }

        if (rand() % 16 == 0)
            mImage.ImageResource.DrawRect(rand() % 800, rand() % 600, rand() % 200, rand() % 150, Colors::RandomColor());
        else
            mImage.ImageResource.DrawFilledCircle(rand() % 800, rand() % 600, rand() % 125, Colors::RandomColor());

    }
    mImage.ImageResource.DumpToPNG("dump.png");
}

Viewport::Viewport()
{
    Name = "Viewport";
    isOpen = false;
}

Viewport::Viewport(const Viewport& other)
 : WindowBase(other) {
    Name = other.Name;
    isOpen = false; // Always start closed
}

void Viewport::Init()
{
    Viewport::Init("Viewport", nullptr);
}

void Viewport::Init(const std::string& WindowName, GInstance* Instance)
{
    WindowBase::Init(WindowName, Instance);

    // Load a canvas to display
    GenerateCanvas();
}

void Viewport::Init(GInstance* GameInstance)
{
    Init("Viewport", GameInstance);
}

void Viewport::Open()
{
    WindowBase::Open();
}

void Viewport::Draw(float deltaTime)
{
    WindowBase::Draw(deltaTime);

    if (ImGui::Begin("Editor", &bEditorOpen))
    {

    }
    ImGui::End();


    static bool window = true;
    ImGui::SetNextWindowSize(ImVec2(1320, 750));

    if (ImGui::Begin("Viewport", &window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::Image(
            mImage.ImageResource.GetID(),
            ImVec2((float)mImage.ImageResource.Width, (float)mImage.ImageResource.Height),
            ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // tint: use white to display texture unchanged
            ImVec4(0.0f, 0.0f, 0.0f, 0.0f)  // border: transparent (or set to a color you want)
        );

        if (ImGui::Button("Regenerate Canvas"))
        {
            GenerateCanvas();
        }
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
    if (mInstance)
    {
        if (const GPU::VulkanSetup* setup = mInstance->GetVulkanSetup())
        {
            const VkDevice device = setup->device;
            // Ensure GPU finished using descriptors/images
            vkDeviceWaitIdle(device);

            // Destroy the ImGui texture and associated Vulkan resources
            if (mInstance)
            {
                mImage.ImageResource.DestroyTexture(mInstance);
            }

            // Destroy helper-owned resources such as the static sampler
            DestroyVulkanHelpersResources(device);
        }
    }

    bEditorOpen = false;
    WindowBase::Close();
}

void Viewport::Tick(const float deltaTime)
{

}



