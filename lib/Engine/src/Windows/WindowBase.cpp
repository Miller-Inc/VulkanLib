//
// Created by James Miller & Joshua Miller on 9/10/2025.
//

#include "Windows/WindowBase.h"
#include "GInstance.h"
#include "Logger.h"

void WindowBase::Init()
{
    isOpen = true;

    if (mInstance != nullptr)
    {
        mTextures = mInstance->LoadResources(Name);
    } else {
        M_LOGGER(Logger::LogCore, Logger::Warning, "GameInstance is null. Cannot load resources for window: %s", Name.c_str());
    }

    Open(); // Open the window by default after initialization
}

void WindowBase::Init(const std::string& WindowName, GInstance* Instance)
{
    Name = WindowName;
    this->mInstance = Instance;
    WindowBase::Init();
}

void WindowBase::Init(GInstance* GameInstance)
{
    mInstance = GameInstance;
    WindowBase::Init();
}

void WindowBase::Open()
{
    isOpen = true;
}

void WindowBase::Draw(const float deltaTime)
{
    // Base draw does nothing
}

void WindowBase::Close()
{
    isOpen = false;
}

void WindowBase::Tick(float deltaTime)
{
    // Base tick does nothing
}

MVector2 WindowBase::GetMousePosInWindow()
{
    const ImVec2 mousePos = ImGui::GetMousePos();
    const ImVec2 windowPos = ImGui::GetWindowPos();
    auto relativePos = ImVec2(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
    return {relativePos.x, relativePos.y};
}

MVector2 WindowBase::GetMouseDeltaInWindow()
{
    const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
    return {mouseDelta.x, mouseDelta.y};
}
