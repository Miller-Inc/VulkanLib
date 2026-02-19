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

set(USE_VULKAN_SHADERS ON)
if(${USE_VULKAN_SHADERS})
    find_package(Vulkan REQUIRED)
    if (WIN32)
        set(GLSLANG_VALIDATOR_EXECUTABLE "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    elseif(UNIX)
        set(GLSLANG_VALIDATOR_EXECUTABLE "$ENV{VULKAN_SDK}/bin/glslangValidator")
    endif()
#    find_program(GLSLANG_VALIDATOR_EXECUTABLE
#            NAMES glslangValidator glslangValidator.exe
#            HINTS ENV PATH
#    )

    if(NOT GLSLANG_VALIDATOR_EXECUTABLE)
        message(FATAL_ERROR "glslangValidator not found.")
    else()
        message(STATUS "Using glslangValidator: ${GLSLANG_VALIDATOR_EXECUTABLE}")
    endif()
endif()

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
    set(EXE_FLAGS ""
            lib/Engine/include/OperatorsExtentions.h)
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
    lib/Engine/src/BaseTypes/Color.cpp
    lib/Vulkan/src/VulkanViewport.cpp
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
    lib/Vulkan/include/VulkanInclude.h
    lib/Engine/include/UserInteraction/UI_Callbacks.h
    lib/Vulkan/include/VulkanViewport.h
    lib/Engine/include/BaseTypes/Primitives/RenderingPrimitives.h
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

# Setup Vulkan Shaders

if(${USE_VULKAN_SHADERS})
    set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders/vulkan")
    set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders/vulkan")

    file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

    # Also add a global definition so SHADER_DIR is available everywhere
    add_definitions(-DSHADER_DIR=\"${SHADER_BINARY_DIR}\")
    message(STATUS "Shader source directory: ${SHADER_SOURCE_DIR}")

    file(GLOB SHADER_FILES "${SHADER_SOURCE_DIR}/*.vert" "${SHADER_SOURCE_DIR}/*.frag" "${SHADER_SOURCE_DIR}/*.comp")
    message(STATUS "Found shader files: ${SHADER_FILES}")

    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(SHADER_FILENAME ${SHADER_FILE} NAME)
        set(SPIRV_OUTPUT "${SHADER_BINARY_DIR}/${SHADER_FILENAME}.spv")

        add_custom_command(
                OUTPUT ${SPIRV_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E echo "Compiling shader: ${SHADER_FILENAME}"
                COMMAND ${GLSLANG_VALIDATOR_EXECUTABLE} -V ${SHADER_FILE} -o ${SPIRV_OUTPUT}
                DEPENDS ${SHADER_FILE}
                COMMENT "Compiling shader ${SHADER_FILENAME} to SPIR-V"
        )

        list(APPEND SPIRV_SHADER_FILES ${SPIRV_OUTPUT})
    endforeach()

    add_custom_target(Shaders ALL DEPENDS ${SPIRV_SHADER_FILES})

    add_dependencies(MillerVulkanLib Shaders)
    add_dependencies(MillerVulkanLibDyn Shaders)
    add_dependencies(MillerVulkanExec Shaders)
    add_dependencies(MillerVulkanObject Shaders)

    # Make SHADER_DIR available to the C++ sources: point to shader binary directory
    # Use an escaped string so it becomes a valid C string literal when used in code
    foreach(_tgt MillerVulkanLib MillerVulkanLibDyn MillerVulkanObject MillerVulkanExec)
        if(TARGET ${_tgt})
            # Use generator expression to ensure correct config-specific path on multi-config generators
            target_compile_definitions(${_tgt} PRIVATE SHADER_DIR=\"${CMAKE_BINARY_DIR}/shaders/vulkan\")
        endif()
    endforeach()
endif()

# Define logging macros based on build type
# LOGGING_ENABLED: 1 = enabled, 0 = disabled
# LOGGING_LEVEL: 2 = full (debug/development), 1 = minimal (release), 0 = none (shipping / MinSizeRel)
string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lc)
set(LOGGING_ENABLED 0)
set(LOGGING_LEVEL 0)

if(_build_type_lc STREQUAL "debug" OR _build_type_lc MATCHES ".*debug.*" OR _build_type_lc STREQUAL "development" OR _build_type_lc MATCHES ".*dev.*")
    set(LOGGING_ENABLED 1)
    set(LOGGING_LEVEL 2)
elseif(_build_type_lc STREQUAL "release" OR _build_type_lc MATCHES ".*release.*")
    set(LOGGING_ENABLED 1)
    set(LOGGING_LEVEL 1)
elseif(_build_type_lc STREQUAL "minsizerel" OR _build_type_lc MATCHES ".*ship.*" OR _build_type_lc MATCHES ".*shipping.*")
    set(LOGGING_ENABLED 0)
    set(LOGGING_LEVEL 0)
else()
    # Default conservative behavior: enable minimal logging
    set(LOGGING_ENABLED 1)
    set(LOGGING_LEVEL 1)
endif()

message(STATUS "CMake build type='${CMAKE_BUILD_TYPE}' -> LOGGING_ENABLED=${LOGGING_ENABLED}, LOGGING_LEVEL=${LOGGING_LEVEL}")

# Export logging macros to targets
# Remove the old ENABLE_DEBUG_LOGGING usage and use LOGGING_* instead
foreach(_tgt MillerVulkanLib MillerVulkanLibDyn MillerVulkanObject MillerVulkanExec)
    if(TARGET ${_tgt})
        target_compile_definitions(${_tgt} PRIVATE LOGGING_ENABLED=${LOGGING_ENABLED} LOGGING_LEVEL=${LOGGING_LEVEL})
    endif()
endforeach()
