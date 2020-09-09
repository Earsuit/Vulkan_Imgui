#ifndef _IMGUI_H_
#define _IMGUI_H_

#include "VulkanBase.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>

class Imgui{
public:
    Imgui(VulkanBase& base) : vulkan(base) {}

private:
    VkDescriptorPool descriptorPool;
    ImGui_ImplVulkan_InitInfo initInfo{};
    VulkanBase& vulkan;
    std::shared_ptr<GLFWwindow> pWindow;

    void createDescriptorPool();
    void createSwapChain();
};

#endif