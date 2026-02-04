cmake_minimum_required(VERSION 4.0)
project(MillerVulkanLib LANGUAGES CXX)
project(MillerVulkanLibDyn LANGUAGES CXX)
project(MillerVulkanExec LANGUAGES CXX)
project(MillerVulkanObject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Find modern CMake CUDA package (gives CUDA::cudart_static)
find_package(CUDAToolkit REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

find_package(Vulkan REQUIRED)

# Icon Handling
if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/Resources/ico/Windows/app.rc")
    set(APP_ICON_RESOURCE ${APP_ICON_RESOURCE_WINDOWS})
    add_definitions(-DAPP_ICON_RESOURCE="${APP_ICON_RESOURCE}")
    message(STATUS "App icon resource set for Windows: ${APP_ICON_RESOURCE}")
elseif(UNIX)
    set(APP_ICON_RESOURCE "")  # No icon resource for Linux in this example
else()
    set(APP_ICON_RESOURCE "")  # No icon resource for other platforms
endif()


# This assumes the SDL source is available in the third_party/sdl3 directory.
add_subdirectory(third_party/sdl3 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/sdl3_net)
add_subdirectory(third_party/json)


# ****************** Executable Configuration ****************** #
# Determine executable flags based on platform and build type
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    # For Windows Release builds, we use the WIN32 flag to suppress the console window.
    set(EXE_FLAGS WIN32)
    message(STATUS "Building for Windows Release with flags: ${EXE_FLAGS}")
elseif (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Shipping-Win64")
        # For Windows Release builds, we use the WIN32 flag to suppress the console window.
        set(EXE_FLAGS WIN32)
        message(STATUS "Building for Windows Release with flags: ${EXE_FLAGS}")
elseif (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    # For Windows Release builds, we use the WIN32 flag to suppress the console window.
    set(EXE_FLAGS WIN32)
    message(STATUS "Building for Windows Release with flags: ${EXE_FLAGS}")
else()
    set(EXE_FLAGS "")
endif()

# List of source files for the MillerVulkanLib executable
set(MILLER_RENDER_LIB_SOURCES
    lib/Engine/src/InputValidation.cpp
    lib/Engine/src/ResourceLoader.cpp
    lib/Engine/src/BaseTypes/Vector2.cpp
    lib/Vulkan/src/VulkanSetup.cpp
    lib/Engine/src/GInstance.cpp
    lib/Engine/src/Windows/WindowBase.cpp
    lib/Engine/src/Windows/Viewport.cpp
    lib/Vulkan/src/VulkanHelpers.cpp
    lib/Vulkan/src/VulkanImage.cpp
)

# List of header files for the MillerVulkanLib executable
set(MILLER_RENDER_LIB_HEADERS
    lib/Engine/include/Core.h
    lib/Engine/include/BaseTypes/MImage.h
    lib/Engine/include/InputValidation.h
    lib/Engine/include/Logger.h
    lib/Engine/include/MacroDefs.h
    lib/Engine/include/ResourceLoader.h
    lib/Engine/include/BaseTypes/Vector2.h
    lib/Vulkan/include/VulkanSetup.hpp
    lib/Engine/include/GInstance.h
    lib/Engine/include/Windows/WindowBase.h
    lib/Engine/include/Windows/Viewport.h
    lib/Vulkan/include/VulkanHelpers.h
    lib/Vulkan/include/VulkanImage.h
    lib/GenericPlatform/include/GenericImage.h
    lib/Engine/include/BaseTypes/Image.h
    lib/Engine/include/BaseTypes/Color.h
    lib/Engine/src/BaseTypes/Color.cpp
    lib/Vulkan/include/VulkanInclude.h
)

add_library(MillerVulkanObject OBJECT
    ${MILLER_RENDER_LIB_SOURCES}
    ${MILLER_RENDER_LIB_HEADERS}
)

add_library(MillerVulkanLib STATIC
    $<TARGET_OBJECTS:MillerVulkanObject>
)

add_library(MillerVulkanLibDyn SHARED
    $<TARGET_OBJECTS:MillerVulkanObject>
)

# create executable
add_executable(MillerVulkanExec ${EXE_FLAGS}
    main.cpp
)

target_link_libraries(MillerVulkanExec PRIVATE MillerVulkanLib)

# Add the application icon resource if it exists
target_sources(MillerVulkanLib PRIVATE ${APP_ICON_RESOURCE})
target_sources(MillerVulkanExec PRIVATE ${APP_ICON_RESOURCE})
target_sources(MillerVulkanLibDyn PRIVATE ${APP_ICON_RESOURCE})

#static link for MINGW
if (MINGW)
    # link libstdc++ and libgcc statically into the exe
    target_compile_options(MillerVulkanLib PRIVATE -static-libgcc -static-libstdc++)
    target_link_options(MillerVulkanLib PRIVATE -static-libgcc -static-libstdc++)
    target_compile_options(MillerVulkanObject PRIVATE -static-libgcc -static-libstdc++)
    target_link_options(MillerVulkanObject PRIVATE -static-libgcc -static-libstdc++)
    target_compile_options(MillerVulkanLibDyn PRIVATE -static-libgcc -static-libstdc++)
    target_link_options(MillerVulkanLibDyn PRIVATE -static-libgcc -static-libstdc++)
    target_compile_options(MillerVulkanExec PRIVATE -static-libgcc -static-libstdc++)
    target_link_options(MillerVulkanExec PRIVATE -static-libgcc -static-libstdc++)

    # Optional: fully static link everything (will require static system and 3rd-party libs)
    target_link_options(MillerVulkanLib PRIVATE -static)
#    target_link_options(MillerVulkanObject PRIVATE -static)
#    target_link_options(MillerVulkanLibDyn PRIVATE -static)
    target_link_options(MillerVulkanExec PRIVATE -static)
endif()

set_target_properties(MillerVulkanLib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(MillerVulkanObject PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(MillerVulkanLibDyn PROPERTIES POSITION_INDEPENDENT_CODE ON)

# include library headers and statically link the CUDA runtime into the exe
target_include_directories(MillerVulkanLib PRIVATE
        lib
        lib/Engine/include
        lib/GenericPlatform/include
        lib/Vulkan/include
)

target_include_directories(MillerVulkanLibDyn PRIVATE
        lib
        lib/Engine/include
        lib/GenericPlatform/include
        lib/Vulkan/include
)

target_link_libraries(MillerVulkanLib PRIVATE CUDA::cudart_static)
target_link_libraries(MillerVulkanLibDyn PRIVATE CUDA::cudart_static)

target_include_directories(MillerVulkanExec PRIVATE
        lib
        lib/Engine/include
        lib/GenericPlatform/include
        lib/Vulkan/include
)
target_link_libraries(MillerVulkanExec PRIVATE CUDA::cudart_static)

target_include_directories(MillerVulkanObject PRIVATE
        lib
        lib/Engine/include
        lib/GenericPlatform/include
        lib/Vulkan/include
)
target_link_libraries(MillerVulkanObject PRIVATE CUDA::cudart_static)

# Include directories for the library target
target_include_directories(MillerVulkanLib PRIVATE imgui third_party/imgui)
target_include_directories(MillerVulkanLib PRIVATE imgui third_party/imgui/backends)
target_include_directories(MillerVulkanLib PRIVATE stb third_party/stb)
target_include_directories(MillerVulkanLib PRIVATE Miller lib/Engine/include)
target_include_directories(MillerVulkanLib PRIVATE Miller lib/Vulkan/include)
target_include_directories(MillerVulkanLib PRIVATE third_party/json/include)
target_include_directories(MillerVulkanLibDyn PRIVATE imgui third_party/imgui)
target_include_directories(MillerVulkanLibDyn PRIVATE imgui third_party/imgui/backends)
target_include_directories(MillerVulkanLibDyn PRIVATE stb third_party/stb)
target_include_directories(MillerVulkanLibDyn PRIVATE Miller lib/Engine/include)
target_include_directories(MillerVulkanLibDyn PRIVATE Miller lib/Vulkan/include)
target_include_directories(MillerVulkanLibDyn PRIVATE third_party/json/include)
target_include_directories(MillerVulkanObject PRIVATE imgui third_party/imgui)
target_include_directories(MillerVulkanObject PRIVATE imgui third_party/imgui/backends)
target_include_directories(MillerVulkanObject PRIVATE stb third_party/stb)
target_include_directories(MillerVulkanObject PRIVATE Miller lib/Engine/include)
target_include_directories(MillerVulkanObject PRIVATE Miller lib/Vulkan/include)
target_include_directories(MillerVulkanObject PRIVATE third_party/json/include)
target_include_directories(MillerVulkanExec PRIVATE imgui third_party/imgui)
target_include_directories(MillerVulkanExec PRIVATE imgui third_party/imgui/backends)
target_include_directories(MillerVulkanExec PRIVATE stb third_party/stb)
target_include_directories(MillerVulkanExec PRIVATE Miller lib/Engine/include)
target_include_directories(MillerVulkanExec PRIVATE Miller lib/Vulkan/include)
target_include_directories(MillerVulkanExec PRIVATE third_party/json/include)

# Define preprocessor definitions for static linking of SDL3 and SDL3_net
target_compile_definitions(MillerVulkanLib PUBLIC SDL_STATIC SDL3_STATIC SDL3_NET_STATIC)
target_compile_definitions(MillerVulkanLibDyn PUBLIC SDL_STATIC SDL3_STATIC SDL3_NET_STATIC)
target_compile_definitions(MillerVulkanObject PUBLIC SDL_STATIC SDL3_STATIC SDL3_NET_STATIC)
target_compile_definitions(MillerVulkanExec PUBLIC SDL_STATIC SDL3_STATIC SDL3_NET_STATIC)

# ImGui setup
set(IMGUI_DIR third_party/imgui)
set(IMGUI_BACKEND_DIR ${IMGUI_DIR}/backends)

# Add ImGui source files to the library target
target_sources(MillerVulkanLib PRIVATE
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_sdl3.cpp
)
target_sources(MillerVulkanLibDyn PRIVATE
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_sdl3.cpp
)
target_sources(MillerVulkanExec PRIVATE
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_sdl3.cpp
)

# Link to the actual SDL3 and Vulkan libraries.
target_link_libraries(MillerVulkanLib PUBLIC SDL3::SDL3 SDL3_net::SDL3_net Vulkan::Vulkan)
target_link_libraries(MillerVulkanLibDyn PUBLIC SDL3::SDL3 SDL3_net::SDL3_net Vulkan::Vulkan)
target_link_libraries(MillerVulkanObject PUBLIC SDL3::SDL3 SDL3_net::SDL3_net Vulkan::Vulkan)
target_link_libraries(MillerVulkanExec PUBLIC SDL3::SDL3 SDL3_net::SDL3_net Vulkan::Vulkan)

# Link to the nlohmann_json library
target_link_libraries(MillerVulkanLib PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(MillerVulkanLib PUBLIC ${NLOHMANN_JSON_TARGET_NAME})
target_link_libraries(MillerVulkanLibDyn PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(MillerVulkanLibDyn PUBLIC ${NLOHMANN_JSON_TARGET_NAME})
target_link_libraries(MillerVulkanExec PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(MillerVulkanExec PUBLIC ${NLOHMANN_JSON_TARGET_NAME})

# Copy Resources folder to the output directory after building
add_custom_command(TARGET MillerVulkanLib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Resources
        $<TARGET_FILE_DIR:MillerVulkanLib>/Resources
)

# tell the compiler we link the static CUDA runtime
target_compile_definitions(MillerVulkanLib PRIVATE CUDART_STATIC)
