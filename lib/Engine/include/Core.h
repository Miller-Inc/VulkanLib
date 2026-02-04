/// Updated by James Miller on February 3, 2026.
/// This file is meant to be a quick include for any
///     external code that uses the Engine.
///     It should NOT be referenced by anything internal
///     to the Engine, as it may cause circular dependencies.
///     It should also be the platform and API agnostic
///     interface to the Engine, so it should not include
///     any platform or API specific headers. (i.e. no Vulkan, SDL, etc.)

#pragma once
#include "Logger.h"
#include "Windows/WindowBase.h"
#include "MacroDefs.h"
#include "imgui.h"
#include "GInstance.h"