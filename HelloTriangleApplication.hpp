#ifndef _HEELO_TRIANGLE_APPLICATION_H_
#define _HEELO_TRIANGLE_APPLICATION_H_

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <vector>
#include <memory>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

struct deletePwindow
{
    void operator()(GLFWwindow *ptr)
    {
        glfwDestroyWindow(ptr);
        glfwTerminate();
    }
};

class HelloTriangleApplication {
public:
    void run();

private:
    std::unique_ptr<GLFWwindow, deletePwindow> pWindow;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    void initVulkan();
    void mainLoop();
    void cleanup();
    void initWindow();
    void createInstance();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    static VKAPI_ATTR 
            VkBool32 
            VKAPI_CALL 
            debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                          void* pUserData);
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

#endif
