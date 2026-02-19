//
// Created by James Miller on 2/5/2026.
//

#pragma once
#include "MacroDefs.h"
#include <functional>

/// Enum to represent different types of user events
/// that can occur in the UI. This enum uses bit flags
/// to allow for combinations of events if needed. Each
/// event type is assigned a unique bit value, making
/// it easy to check for specific events using bitwise
/// operations. For example, you can check if a left
/// click event occurred by checking if the eventType
/// has the UI_Event_LeftClick bit set. This enum can
/// be extended with additional event types as needed
/// to cover more user interactions in the UI, such as
/// touch events, gesture events, etc. The UI_Event_None
/// value can be used to indicate that no event occurred
/// or to initialize the eventType field in the
/// UserEventData struct to a default state.
enum UserEventType : uint32
{
    UI_Event_None = 0,
    UI_Event_Quit = 1 << 0,
    UI_Event_LeftClick = 1 << 1,
    UI_Event_Drag = 1 << 2,
    UI_Event_Hover = 1 << 3,
    UI_Event_Release = 1 << 4,
    UI_Event_Keypress = 1 << 5,
    UI_Event_KeyUp = 1 << 6,
    UI_Event_RightClick = 1 << 7,
    UI_Event_Scroll = 1 << 8,
    UI_Event_MiddleClick = 1 << 9,
    UI_Event_DoubleClick = 1 << 10,
    UI_Event_Resize = 1 << 11,
    UI_Event_GamepadEvent = 1 << 12,
};

/// Struct to hold data for user events, such as mouse clicks,
/// drags, key presses, etc. This struct can be extended with
/// additional fields as needed to capture more specific information
/// about the event. The eventType field indicates the type of event
/// that occurred, and the other fields provide relevant data based
/// on the event type (e.g., mouse position for click events,
/// key code for key presses, etc.). This struct can be used
/// in callback functions to handle user interactions in the UI.
typedef struct UserEventData
{
    // Add any relevant data for the user event here
    uint32 eventType = UI_Event_None;
    // For mouse events
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    // For key events
    uint32 keyCode = 0;
    // For scroll events
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    // For gamepad events
    uint32 gamepadButton = 0;
    float gamepadLeftAxisX = 0.0f;
    float gamepadLeftAxisY = 0.0f;
    float gamepadRightAxisX = 0.0f;
    float gamepadRightAxisY = 0.0f;
    float gamepadTriggerLeft = 0.0f;
    float gamepadTriggerRight = 0.0f;
} UserEventData;

/// Type definition for a user event callback function. This is a
/// function pointer type that takes a reference to a UserEventData
/// struct as an argument. This allows you to define callback
/// functions that can be called when a user event occurs, passing
/// relevant event data to the callback for handling. You can use
/// this type to create a flexible event handling system in your UI,
/// allowing different parts of your application to respond to user
/// interactions in a modular way.
typedef std::function<void(const UserEventData&)> UserEventCallback;