cmake_minimum_required(VERSION 3.16)
project(Neon)

add_compile_definitions(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

set(CMAKE_CXX_STANDARD 17)

include_directories($ENV{VULKAN_SDK})
include_directories($ENV{VULKAN_SDK}/Include)
include_directories(src)
include_directories(src/Core)
include_directories(src/Event)
include_directories(src/Renderer)
include_directories(src/Tools)
include_directories(src/Window)
include_directories(vendor/allocator)
include_directories(vendor/glfw-3.3.2/include)
include_directories(vendor/glm-0.9.9.7)
include_directories(vendor/imgui)
include_directories(vendor/loader)

add_executable(Neon
        src/Core/Allocator.cpp
        src/Core/ImGuiLayer.cpp
        src/Core/Input.cpp
        src/Core/Layer.cpp
        src/Core/LayerStack.cpp
        src/Renderer/DescriptorPool.cpp
        src/Renderer/DescriptorSetType.cpp
        src/Renderer/GraphicsPipeline.cpp
        src/Renderer/ObjModel.cpp
        src/Renderer/PerspectiveCamera.cpp
        src/Renderer/PerspectiveCameraController.cpp
        src/Renderer/RenderPass.cpp
        src/Renderer/VulkanRenderer.cpp
        src/Renderer/VulkanShader.cpp
        src/Tools/FileTools.cpp
        src/Tools/VulkanTools.cpp
        src/Window/Window.cpp
        src/Application.cpp
        src/main.cpp
        src/Sandbox3D.cpp

        vendor/imgui/examples/imgui_impl_glfw.cpp
        vendor/imgui/examples/imgui_impl_vulkan.cpp
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_widgets.cpp)

target_link_libraries(Neon $ENV{VULKAN_SDK}/lib/vulkan-1.lib ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw-3.3.2/lib-vc2019/glfw3.lib)

target_precompile_headers(Neon PUBLIC src/neopch.h)
