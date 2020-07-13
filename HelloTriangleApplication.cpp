#include "HelloTriangleApplication.hpp"

#include <cstdint> // Necessary for UINT32_MAX
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <unistd.h>

// The VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    // vkCreateDebugUtilsMessengerEXT function is an extension function,
    // it is not automatically loaded. We have to look up its address ourselves using vkGetInstanceProcAddr.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// load the binary data from shader files
static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    // read at the end of the file is that
    // we can use the read position to determine the size of the file and allocate a buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow()
{
    // initializes the GLFW library
    glfwInit();
    // Because GLFW was originally designed to create an OpenGL context,
    // we need to tell it to not create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // disable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create the actual window
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();  // after logical device creation
    createImageViews(); // after swap chain creation.
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSemaphores();
}

void HelloTriangleApplication::createInstance()
{
    //first check validation layers
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // optional
    // fill in a struct with some information about our application
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // necessary
    // tells the Vulkan driver which global extensions and
    // validation layers we want to use.
    // global here means that they apply to the entire program and not a specific device
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // specify the desired global extensions
    auto reExtensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(reExtensions.size());
    ;
    createInfo.ppEnabledExtensionNames = reExtensions.data();

    std::cout << "required extensions:\n";
    for (const auto& extension : reExtensions) {
        std::cout << '\t' << extension << '\n';
    }

    // To retrieve a list of supported extensions before creating an instance
    // First get to know how many there are
    // We don't really need to check for the existence of this extension,
    // because it should be implied by the availability of the validation layers
    // if DEBUG is on
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::cout << "available extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    // ensure it is not destroyed before the vkCreateInstance call.
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    // global validation layers
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void HelloTriangleApplication::mainLoop()
{
    // checks for events like pressing the X button until the window has been closed by the user
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void HelloTriangleApplication::cleanup()
{
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // should be cleaned up before the device
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

bool HelloTriangleApplication::checkValidationLayerSupport()
{
    // list all of the available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // retrieve a list of supported validation layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
    // these are the extensions required by the GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    // GLFW has a handy built-in function that returns the extension(s) it needs to do
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangleApplication::setupDebugMessenger()
{
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void HelloTriangleApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
}

void HelloTriangleApplication::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // because not all graphics cards are created equal
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device)
{
    // basic device properties like the name, type and supported Vulkan version
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // optional features like texture compression, 64 bit floats and multi viewport rendering (useful for VR)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // Swap chain support is sufficient for this tutorial
    // if there is at least one supported image format and
    // one supported presentation mode given the window surface we have.
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        // It is important that we only try to query for swap chain support after verifying that the extension is available
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           /*deviceFeatures.geometryShader &&*/ // This festure doesn't work on my Mac
           findQueueFamilies(device).isComplete() &&
           extensionsSupported &&
           swapChainAdequate;
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // We need to find at least one queue family that has graphics capabilities
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // check it has the capability of presenting to our window surface
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }


    return indices;
}

void HelloTriangleApplication::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority; // required even if there is only a single queue
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Right now we don't need anything special
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    // Because we're only creating a single queue from this family, we'll simply use index 0
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void HelloTriangleApplication::createSurface()
{
    // The glfwCreateWindowSurface function performs exactly this operation with a different implementation for each platform.
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        // Here we choose the most common SRGB color format: VK_FORMAT_B8G8R8A8_SRGB
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // If that also fails then we could start ranking the available formats based on how "good" they are,
    // but in most cases it's okay to just settle with the first format that is specified.
    return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // prefer triple buffering when supported
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    // check if the surface allows to dismatch the resolution of the window
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = {WIDTH, HEIGHT};

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void HelloTriangleApplication::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // minimium number of imnages in the swap chain it requires to function.
    // However, simply sticking to this minimum means that we may sometimes have to
    // wait on the driver to complete internal operations before we can acquire another image to render to.
    // it is recommended to request at least one more image than the minimum
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // make sure to not exceed the maximum number of images.
    // 0 is a special value that means that there is no maximum
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    // specifies the amount of layers each image consists of
    // This is always 1 unless you are developing a stereoscopic 3D application.
    createInfo.imageArrayLayers = 1;
    // what kind of operations we'll use the images in the swap chain for.
    // In this tutorial we're going to render directly to them,
    // which means that they're used as color attachment.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    //  specify how to handle swap chain images that will be used across multiple queue families.
    // EXCLUSIVE mode if they are the same
    if (indices.graphicsFamily != indices.presentFamily) {
        // Concurrent mode requires you to specify in advance between which queue families ownership will be shared
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // For this tutorial, we don't want any transformation
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // specifies if the alpha channel should be used for blending with other windows in the window system.
    // You'll almost always want to simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // The presentMode member speaks for itself.
    // If the clipped member is set to VK_TRUE
    // then that means that we don't care about the color of pixels that are obscured,
    // for example because another window is in front of them.
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // retrieving the handles of the VkImages in the swap chain
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void HelloTriangleApplication::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        // The viewType and format fields specify how the image data should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        // stick to the default mapping
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // describes what the image's purpose is and which part of the image should be accessed.
        // Our images will be used as color targets without any mipmapping levels or multiple layers.
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code)
{
    // Before we can pass the code to the pipeline, we have to wrap it in a VkShaderModule object
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // the size of the bytecode is specified in bytes,
    // but the bytecode pointer is a uint32_t pointer rather than a char pointer.
    // When you perform a cast like this,
    // you also need to ensure that the data satisfies the alignment requirements of uint32_t.
    // Luckyly, the data is stored in an std::vector where the default allocator already ensures
    // that the data satisfies the worst case alignment requirements.
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void HelloTriangleApplication::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; // the function to invoke, known as the entrypoint.

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Because we're hard coding the vertex data directly in the vertex shader,
    // we'll fill in this structure to specify that there is no vertex data to load for now
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    // specify the range of depth values to use for the framebuffer
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // If depthClampEnable is set to VK_TRUE,
    // then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them.
    rasterizer.depthClampEnable = VK_FALSE;
    // If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage.
    // This basically disables any output to the framebuffer.
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // determines how fragments are generated for geometry.
    // Using any mode other than fill requires enabling a GPU feature.
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // describes the thickness of lines in terms of number of fragments.
    // The maximum line width that is supported depends on the hardware
    // and any line thicker than 1.0f requires you to enable the wideLines GPU feature
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f;          // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = nullptr;            // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    // It is designed to take multiple VkGraphicsPipelineCreateInfo objects and create multiple VkPipeline objects in a single call.
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // As these two shader modules are local variables, cleaned here
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void HelloTriangleApplication::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    // The format of the color attachment should match the format of the swap chain images
    colorAttachment.format = swapChainImageFormat;
    // we're not doing anything with multisampling yet, so we'll stick to 1 sample
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering.
    // In our case we're going to use the clear operation to clear the framebuffer to black before drawing a new frame
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // We're interested in seeing the rendered triangle on the screen, so we're going with the store operation here.
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Our application won't do anything with the stencil buffer, so the results of loading and storing are irrelevant
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Using VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in.
    // The caveat of this special value is that the contents of the image are not guaranteed to be preserved,
    // but that doesn't matter since we're going to clear it anyway
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // We want the image to be ready for presentation using the swap chain after rendering
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    // specifies which attachment to reference by its index in the attachment descriptions array
    // Our array consists of a single VkAttachmentDescription, so its index is 0
    colorAttachmentRef.attachment = 0;
    // The layout specifies which layout we would like the attachment to have during a subpass that uses this reference
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    // Vulkan may also support compute subpasses in the future, so we have to be explicit about this being a graphics subpass
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // The index of the attachment in this array is directly referenced from
    // the fragment shader with the layout(location = 0) out vec4 outColor directive!
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    // The first two fields specify the indices of the dependency and the dependent subpass.
    // The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after
    // the render pass depending on whether it is specified in srcSubpass or dstSubpass.
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    // The next two fields specify the operations to wait on and the stages in which these operations occur.
    // We need to wait for the swap chain to finish reading from the image before we can access it.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    // The operations that should wait on this are in the color attachment stage and involve the writing of the color attachment.
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void HelloTriangleApplication::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void HelloTriangleApplication::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    // We will only record the command buffers at the beginning of the program and
    // then execute them many times in the main loop, so we're not going to use either of these flags
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void HelloTriangleApplication::createCommandBuffers()
{
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // Begin recording a command buffer
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // Starting a render pass
        VkRenderPassBeginInfo renderPassInfo{};
        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        // We created a framebuffer for each swap chain image that specifies it as color attachment.
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        // It should match the size of the attachments for best performance.
        renderPassInfo.renderArea.extent = swapChainExtent;
        // The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR,
        // which we used as load operation for the color attachment
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Basic drawing commands
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        // We've now told Vulkan which operations to execute in the graphics pipeline and
        // which attachment to use in the fragment shader, so all that remains is telling it to draw the triangle
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void HelloTriangleApplication::createSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

        throw std::runtime_error("failed to create semaphores!");
    }
}

void HelloTriangleApplication::drawFrame()
{
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo submitInfo{};
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // The first three parameters specify which semaphores to wait on before execution begins and
    // in which stage(s) of the pipeline to wait. Each entry in the waitStages array corresponds to
    // the semaphore with the same index in pWaitSemaphores.
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // The next two parameters specify which command buffers to actually submit for execution.
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    // The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to
    // signal once the command buffer(s) have finished execution
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // The last parameter references an optional fence that will be signaled when the command buffers finish execution.
    // We're using semaphores for synchronization, so we'll just pass a VK_NULL_HANDLE
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    // The next two parameters specify the swap chains to present images to and the index of the image for each swap chain.
    // This will almost always be a single one.
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    // It allows you to specify an array of VkResult values to check for every individual swap chain if presentation was successful.
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(presentQueue, &presentInfo);
}
