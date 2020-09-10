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
    Imgui(VulkanBase& base);
    void init();
    void initVulkanResource(VkRenderPass renderPass);

private:
    VkDescriptorPool descriptorPool;
    VulkanBase& vulkan;

    void createDescriptorPool();
    void uploadFont();
};

#endif