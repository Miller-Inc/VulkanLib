# Miller Vulkan Library

## Features
- GPU-accelerated computations for high performance
- Object-oriented design for easy integration
- Similar to Unreal Engine's C++ API for familiarity
- Modular architecture for extensibility

> Note: This project is currently in the early stages of development 
> and has only been tested on Windows with NVIDIA GPUs. Future updates 
> will include support for additional platforms and hardware (e.g., Ubuntu, 
> RHEL, & Vulkan). Mac support is not planned at this time.

## Getting Started
To get started, follow these steps:

> Note: This project is currently optimized for NVIDIA GPUs using Vulkan. 
> Support for other platforms and APIs (such as OpenCL or CUDA) may 
> be added in future releases. 

1. Ensure you have the following prerequisites installed:
   - C++20 compatible compiler
   - CMake 4.0 or higher
   - Vulkan SDK 1.4 or higher

2. Clone the repository:
   ```bash
   # SSH
   git clone git@github.com:Miller-Inc/VulkanLib.git --recurse-submodules
   
    # HTTPS
    git clone https://github.com/Miller-Inc/VulkanLib.git
    ```
   
3. Navigate to the project directory:
   ```bash
   cd VulkanLib
   ```

4. Build the project using CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```
   
## Current Status & Progress
The Miller Vulkan Library is in the early stages of development.
Please check back soon for real status updates and progress reports.