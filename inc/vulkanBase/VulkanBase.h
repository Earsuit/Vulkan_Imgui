#ifndef _HEELO_TRIANGLE_APPLICATION_H_
#define _HEELO_TRIANGLE_APPLICATION_H_

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <cstdlib>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct deletePwindow {
    void operator()(GLFWwindow* ptr)
    {
        glfwDestroyWindow(ptr);
        glfwTerminate();
    }
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() &&
               presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanBase {
public:
    std::unique_ptr<GLFWwindow, deletePwindow> pWindow;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device; //logical device
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkAllocationCallbacks* allocator = nullptr;
    uint32_t minImageCount;
    uint32_t swapChainImageCount;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    bool framebufferResized = false;

    VulkanBase(uint32_t width, uint32_t height, const std::string, bool enableValidationLayers);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    std::optional<uint32_t> prepareFrame();
    void submitFrame(uint32_t imageIndex, VkSemaphore* signalSemaphores);

    virtual void prepare();

protected:
    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createImage(uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image,
                               VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkImageView createImageView(VkImage image, VkFormat format);

    virtual void initVulkan();
    virtual void cleanup();

private:
    bool enableValidationLayers;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    size_t currentFrame = 0;

    void initWindow(uint32_t width, uint32_t height, const std::string title);
    void createInstance();
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    void createLogicalDevice();
    void createSurface();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void createSwapChain();
    void createImageViews();
    void createCommandPool();
    void createCommandBuffers();
    void recreateSwapChain();
    void cleanupSwapChain();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);    
};

#endif
