

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include <imgui_operators.h>

#include "Core.hpp"
#include "CoreCallbacks.hpp"
#include "CoreConstants.hpp"
#include "VulkanHelpers.hpp"

#include <ShellScalingApi.h>

#include <cassert>
#include <vector>
#include <algorithm>
#include <Timer.hpp>

#include <glm/glm.hpp>

namespace win {

    int    windowWidth, windowHeight; 
    HWND   hwnd;
    double refreshInterval = 1.0 / 60; // 1 / hz   hz = monitor refresh rate
    float  windowDpi       = 0.0;
    bool   windowMinimized = false;

}
namespace vlkn {
    
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR             surface;
    uint32_t                 graphicsQueueIndex;
    uint32_t                 presentQueueIndex;
    VkPhysicalDevice         physicalDevice;
    VkDevice                 device;
    VkQueue                  graphicsQueue; 
    VkQueue                  presentQueue;
    VkExtent2D               windowImageExtent;
    VkSwapchainKHR           swapchain; 
    uint32_t                 imageCount          = 0;   
    VkImage*                 swapchainImages     = nullptr;
    VkImageView*             swapchainImageViews = nullptr;
    VkRenderPass             renderPass; 
    VkFramebuffer*           framebuffers        = nullptr; 

    namespace vp {

        VkImage          image;
        VkDeviceMemory   imageMem;
        VkImageView      imageView;
        VkRenderPass     renderPass;
        VkFramebuffer    framebuffer;
        VkPipelineLayout pipelineLayout; 
        VkPipeline       graphicsPipeline;
        VkBuffer         vertBuff;
        VkDeviceMemory   vertBuffMem;
        VkSampler        frameSampler; 
        ImTextureID      framebufferTexID;

    }

    VkCommandPool            commandPool; 
    VkCommandBuffer          commandBuff; 
    VkDescriptorPool         descriptorPool;
    VkSemaphore              renderDoneSemaphore;
    VkSemaphore              imageReadySemaphore;
    VkFence                  frameFinishedFence;

}
GuiSizes s_gui;

struct Vertex {
    glm::vec3 pos; 
};
Vertex vertices[] = {
    { { -0.5f,  0.5f,  0.0f } },
    { {  0.0f, -0.5f,  0.0f } },
    { {  0.5f,  0.5f,  0.0f } },
}; 

// static funcs
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
static void check_vk_result(VkResult err)
{
    assert(err == VK_SUCCESS && "check_vk_result failed"); 
}

static void createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE) {

    // Swapchain creation 
    {

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.flags = 0;
        createInfo.surface = vlkn::surface;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vlkn::physicalDevice, vlkn::surface, &capabilities);

        assert(c_vlkn::imageCount >= capabilities.minImageCount);

        createInfo.minImageCount = c_vlkn::imageCount;
        createInfo.imageFormat = c_vlkn::format;
        createInfo.imageColorSpace = c_vlkn::colorSpace;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            createInfo.imageExtent = capabilities.currentExtent;
        }
        else {
            createInfo.imageExtent = {
                std::clamp((unsigned)win::windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp((unsigned)win::windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };

        }

        vlkn::windowImageExtent = createInfo.imageExtent;

        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { vlkn::graphicsQueueIndex, vlkn::presentQueueIndex };
        if (vlkn::graphicsQueueIndex == vlkn::presentQueueIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
        }
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = c_vlkn::presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult err = vkCreateSwapchainKHR(vlkn::device, &createInfo, nullptr, &vlkn::swapchain);
        assert(err == VK_SUCCESS && "Swapchain creation failed");

    }

    // Swap chain images
    {

        vkGetSwapchainImagesKHR(vlkn::device, vlkn::swapchain, &vlkn::imageCount, nullptr);
        vlkn::swapchainImages = new VkImage[vlkn::imageCount];
        vkGetSwapchainImagesKHR(vlkn::device, vlkn::swapchain, &vlkn::imageCount, vlkn::swapchainImages);

    }

    // Swap chain image views
    {

        vlkn::swapchainImageViews = new VkImageView[vlkn::imageCount];
        for (size_t i = 0; i < vlkn::imageCount; ++i) {

            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.flags = 0;
            createInfo.image = vlkn::swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = c_vlkn::format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult err = vkCreateImageView(vlkn::device, &createInfo, nullptr, &vlkn::swapchainImageViews[i]);
            assert(err == VK_SUCCESS && "Image view creation failed");

        }

    }

}
static void createFramebuffers() {

    vlkn::framebuffers = new VkFramebuffer[vlkn::imageCount];

    for (uint32_t i = 0; i < vlkn::imageCount; ++i) {

        VkImageView attachments[] = { vlkn::swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vlkn::renderPass;
        framebufferInfo.attachmentCount = arraySize(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vlkn::windowImageExtent.width;
        framebufferInfo.height = vlkn::windowImageExtent.height;
        framebufferInfo.layers = 1;

        VkResult err = vkCreateFramebuffer(vlkn::device, &framebufferInfo, nullptr, &vlkn::framebuffers[i]);
        assert(err == VK_SUCCESS && "Frambuffer creation failed");

    }

}
static void cleanupSwapchainResources() {

    for (unsigned i = 0; i < vlkn::imageCount; ++i) {
        vkDestroyFramebuffer(vlkn::device, vlkn::framebuffers[i], nullptr);
    }
    delete[] vlkn::framebuffers;
    for (unsigned i = 0; i < vlkn::imageCount; ++i) {
        vkDestroyImageView(vlkn::device, vlkn::swapchainImageViews[i], nullptr);
    }
    delete[] vlkn::swapchainImageViews;
    delete[] vlkn::swapchainImages;

}
void recreateSwapchain() {

    vkDeviceWaitIdle(vlkn::device); 

    cleanupSwapchainResources(); 
    vkDestroySwapchainKHR(vlkn::device, vlkn::swapchain, nullptr);

    createSwapchain();
    createFramebuffers(); 

}

void App::init(HINSTANCE hInstance, int nCmdShow) {

    // Init Variables 
    const char* desiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

    // Window creation
    {
        SetProcessDPIAware();

        const wchar_t CLASS_NAME[] = L"Main Window";

        WNDCLASS wc = { };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        HMONITOR mntr = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        UINT dpiX, dpiY;
        GetDpiForMonitor(mntr, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        win::windowDpi = float(dpiX / 96.0);

        int mntrWidth = GetSystemMetrics(SM_CXSCREEN);
        int mntrHeight = GetSystemMetrics(SM_CYSCREEN);

        assert(c_WindowPercentSize > 0.0 && c_WindowPercentSize <= 1.0);
        assert(c_WindowAspectRatio > 0.0);

        if (mntrWidth > mntrHeight) {
            win::windowHeight = int(c_WindowPercentSize * mntrHeight);
            win::windowWidth = int(c_WindowAspectRatio * win::windowHeight);
        }
        else {
            win::windowWidth = int(c_WindowPercentSize * mntrWidth);
            win::windowHeight = int(win::windowWidth / c_WindowAspectRatio);
        }

        int startPosX = int(0.5 * (mntrWidth - win::windowWidth));
        int startPosY = int(0.5 * (mntrHeight - win::windowHeight));

        win::hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            L"Vulkan App",
            WS_OVERLAPPEDWINDOW,
            startPosX,
            startPosY,
            win::windowWidth,
            win::windowHeight,
            NULL,
            NULL,
            hInstance,
            NULL
        );

        assert(win::hwnd != NULL && "Window Creation Failed");

        DEVMODE devMode;
        devMode.dmSize = sizeof(devMode);
        BOOL err = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode); 
        assert(err && "EnumDisplaySettings failed");
        win::refreshInterval = 1.0 / devMode.dmDisplayFrequency;

    }

    // Vulkan Instance creation 
    {

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "SimpleViewer3D";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "No Engine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_3;

        const char* extensionNames[] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface",
#ifdef ENABLE_VK_VALIDATION_LAYERS
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif 
        };

        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        for (const char* extensionName : extensionNames) {
            for (const VkExtensionProperties& extension : extensions) {
                if (strcmp(extension.extensionName, extensionName) == 0) {
                    goto extensionExists;
                }
            }
            assert(!"An extension in entensionNames does not exist.");
        extensionExists:;
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo        = &appInfo;
        createInfo.enabledExtensionCount   = arraySize(extensionNames);
        createInfo.ppEnabledExtensionNames = extensionNames;

#ifdef ENABLE_VK_VALIDATION_LAYERS

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* desiredLayer : desiredLayers) {
            for (const VkLayerProperties& layer : availableLayers) {
                if (strcmp(desiredLayer, layer.layerName) == 0) {
                    goto layerExists;
                }

            }
            assert(!"A validation Layer in desiredLayers does not exist.");
        layerExists:;
        }
        createInfo.enabledLayerCount   = arraySize(desiredLayers);
        createInfo.ppEnabledLayerNames = desiredLayers;

        VkDebugUtilsMessengerCreateInfoEXT validationCreateInfo{};
        validationCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        validationCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT /*| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT */ | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        validationCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        validationCreateInfo.pfnUserCallback = validationLayerCallback;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&validationCreateInfo;
#else
        createInfo.enabledLayerCount   = 0;
        createInfo.ppEnabledLayerNames = nullptr; 
        createInfo.pNext               = nullptr;
#endif

        VkResult err = vkCreateInstance(&createInfo, nullptr, &vlkn::instance);
        assert(err == VK_SUCCESS && "Vulkan instance creation failed");

#ifdef ENABLE_VK_VALIDATION_LAYERS

        err = CreateDebugUtilsMessengerEXT(vlkn::instance, &validationCreateInfo, nullptr, &vlkn::debugMessenger);
        assert(err == VK_SUCCESS && "CreateDebugUtilsMessengerEXT failed");

#endif 
    }

    // Vulkan surface creation
    {

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = win::hwnd;
        createInfo.hinstance = hInstance;

        VkResult err = vkCreateWin32SurfaceKHR(vlkn::instance, &createInfo, nullptr, &vlkn::surface);
        assert(err == VK_SUCCESS && "Surface creation failed");

    }

    // Pick physical device
    {

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vlkn::instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(vlkn::instance, &deviceCount, physicalDevices.data());
        assert(deviceCount > 0);

        std::vector<VkPhysicalDevice> compatableDevices;
        compatableDevices.reserve(deviceCount);

        for (VkPhysicalDevice queriedDevice : physicalDevices) {

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(queriedDevice, &deviceProperties);

            uint32_t queueFamilyPropertyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(queriedDevice, &queueFamilyPropertyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(queriedDevice, &queueFamilyPropertyCount, queueFamilies.data());

            bool hasGraphicsQueue = false;
            bool hasPresentQueue = false;
            unsigned i = 0;
            for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    hasGraphicsQueue = true;

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(queriedDevice, i, vlkn::surface, &presentSupport);
                if (presentSupport)
                    hasPresentQueue = true;

                ++i;
            }
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(queriedDevice, nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(queriedDevice, nullptr, &extensionCount, availableExtensions.data());

            bool hasSwapchain = false;
            for (const VkExtensionProperties& extension : availableExtensions) {
                if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    hasSwapchain = true;
                    break;
                }
            }

            bool formatsAdequate = false;

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(queriedDevice, vlkn::surface, &formatCount, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(queriedDevice, vlkn::surface, &formatCount, formats.data());
            for (const VkSurfaceFormatKHR& format : formats) {
                if (format.format == c_vlkn::format && format.colorSpace == c_vlkn::colorSpace) {
                    formatsAdequate = true;
                    break;
                }
            }

            uint32_t presentModeCount;
            bool presentModeAdequate = false;
            vkGetPhysicalDeviceSurfacePresentModesKHR(queriedDevice, vlkn::surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(queriedDevice, vlkn::surface, &presentModeCount, presentModes.data());
            for (VkPresentModeKHR presentMode : presentModes) {
                if (presentMode == c_vlkn::presentMode) {
                    presentModeAdequate = true;
                    break;
                }
            }

            if (hasPresentQueue && hasGraphicsQueue && hasSwapchain && formatsAdequate && presentModeAdequate) {
                compatableDevices.push_back(queriedDevice);
            }
        }
        assert(compatableDevices.size() > 0);
        for (VkPhysicalDevice compatableDevice : compatableDevices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(compatableDevice, &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                vlkn::physicalDevice = compatableDevice;
                goto discreteFound;
            }
        }
        vlkn::physicalDevice = compatableDevices[0];
    discreteFound:;

    }

    // Logical device creation
    {

        VkDeviceQueueCreateInfo queueCreateInfos[2]{};

        float queuePriority = 1.0;
        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].queueFamilyIndex = vlkn::graphicsQueueIndex;
        queueCreateInfos[0].queueCount       = 1;
        queueCreateInfos[0].pQueuePriorities = &queuePriority;

        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].queueFamilyIndex = vlkn::presentQueueIndex;
        queueCreateInfos[1].queueCount       = 1;
        queueCreateInfos[1].pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos    = queueCreateInfos;
        createInfo.queueCreateInfoCount = arraySize(queueCreateInfos);
        if (vlkn::graphicsQueueIndex == vlkn::presentQueueIndex) createInfo.queueCreateInfoCount -= 1;

        VkPhysicalDeviceFeatures deviceFeatures{};
        createInfo.pEnabledFeatures = &deviceFeatures;
        const char* extensionNames[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        createInfo.ppEnabledExtensionNames = extensionNames;
        createInfo.enabledExtensionCount   = arraySize(extensionNames);

#ifdef ENABLE_VK_VALIDATION_LAYERS
        createInfo.enabledLayerCount   = arraySize(desiredLayers);
        createInfo.ppEnabledLayerNames = desiredLayers;
#else
        createInfo.enabledLayerCount = 0;
#endif

        VkResult err = vkCreateDevice(vlkn::physicalDevice, &createInfo, nullptr, &vlkn::device);
        assert(err == VK_SUCCESS && "Logical device creation failed");


    }

    // Queue retrieval
    {

        vkGetDeviceQueue(vlkn::device, vlkn::graphicsQueueIndex, 0, &vlkn::graphicsQueue);
        vkGetDeviceQueue(vlkn::device, vlkn::presentQueueIndex, 0, &vlkn::presentQueue);

    }

    createSwapchain(VK_NULL_HANDLE); 

    // Render pass creation 
    {

        VkAttachmentDescription colorAttachment{};
        colorAttachment.flags          = 0;
        colorAttachment.format         = c_vlkn::format;
        colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.pResolveAttachments     = nullptr;

        VkSubpassDependency dependency{}; 
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[] = { colorAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.flags           = 0;
        renderPassInfo.attachmentCount = arraySize(attachments);
        renderPassInfo.pAttachments    = attachments;
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        VkResult err = vkCreateRenderPass(vlkn::device, &renderPassInfo, nullptr, &vlkn::renderPass);
        assert(err == VK_SUCCESS && "Render pass creation failed");

    }

    createFramebuffers();

    // Command Pool Creation 
    {

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = vlkn::graphicsQueueIndex;

        VkResult err = vkCreateCommandPool(vlkn::device, &createInfo, nullptr, &vlkn::commandPool);
        assert(err == VK_SUCCESS && "Command pool creation failed");

    }

    {} // Fixes visual studio bug with collapsing scopes

    // Command buffer creation
    {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = vlkn::commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult err = vkAllocateCommandBuffers(vlkn::device, &allocInfo, &vlkn::commandBuff);
        assert(err == VK_SUCCESS && "Command buffer allocation failed");

    }

    // Decriptor pool creation 
    {

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets       = 2;
        poolInfo.poolSizeCount = 0;
        poolInfo.pPoolSizes    = nullptr;

        VkResult err = vkCreateDescriptorPool(vlkn::device, &poolInfo, nullptr, &vlkn::descriptorPool);
        assert(err == VK_SUCCESS && "Descriptor pool creation failed");

    }

    // Semaphore and Fence creation 
    {

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult err1 = vkCreateSemaphore(vlkn::device, &semaphoreInfo, nullptr, &vlkn::imageReadySemaphore);
        VkResult err2 = vkCreateSemaphore(vlkn::device, &semaphoreInfo, nullptr, &vlkn::renderDoneSemaphore);
        assert((err1 == VK_SUCCESS || err2 == VK_SUCCESS) && "Semaphore creation failed");

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult err = vkCreateFence(vlkn::device, &fenceInfo, nullptr, &vlkn::frameFinishedFence);
        assert(err == VK_SUCCESS && "Fence Creation failed");

    }

    // Imgui setup 
    {

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; 
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    

        ImGui_ImplWin32_Init(win::hwnd);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance        = vlkn::instance;
        initInfo.PhysicalDevice  = vlkn::physicalDevice;
        initInfo.Device          = vlkn::device;
        initInfo.QueueFamily     = vlkn::graphicsQueueIndex;
        initInfo.Queue           = vlkn::graphicsQueue;
        initInfo.PipelineCache   = VK_NULL_HANDLE;
        initInfo.DescriptorPool  = vlkn::descriptorPool;
        initInfo.RenderPass      = vlkn::renderPass; 
        initInfo.Subpass         = 0;
        initInfo.MinImageCount   = c_vlkn::imageCount;
        initInfo.ImageCount      = c_vlkn::imageCount;
        initInfo.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator       = nullptr;
        initInfo.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&initInfo);

        for (int i = 0; i < (sizeof(GuiSizes) / sizeof(float)); ++i) {
            ((float*)&s_gui)[i] *= win::windowDpi;
        }

        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", s_gui.fontSize);
        io.Fonts->Build();

    }

    // viewport (NOT imgui viewport as in a separate window). This is where the 3D model is drawn.
    {

        // Image creation
        {

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.flags = 0;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = c_vlkn::format;
            imageInfo.extent = { c_vlkn::vpWidth, c_vlkn::vpHeight, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.queueFamilyIndexCount = 1;
            imageInfo.pQueueFamilyIndices = &vlkn::graphicsQueueIndex;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkResult err = vkCreateImage(vlkn::device, &imageInfo, nullptr, &vlkn::vp::image);

        }

        // Image memory creation
        {

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(vlkn::device, vlkn::vp::image, &memRequirements);
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(vlkn::physicalDevice, &memProps);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;

            uint32_t i = 0;
            for (; i < memProps.memoryTypeCount; i++) {
                if (memRequirements.memoryTypeBits & (1 << i) && memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    break;
                }
            }

            allocInfo.memoryTypeIndex = i;
            VkResult err = vkAllocateMemory(vlkn::device, &allocInfo, nullptr, &vlkn::vp::imageMem);
            assert(err == VK_SUCCESS && "Buffer allocation failed");

            vkBindImageMemory(vlkn::device, vlkn::vp::image, vlkn::vp::imageMem, 0);

        }

        // Image view creation
        {

            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.flags = 0;
            createInfo.image = vlkn::vp::image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = c_vlkn::format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult err = vkCreateImageView(vlkn::device, &createInfo, nullptr, &vlkn::vp::imageView);
            assert(err == VK_SUCCESS && "Image view creation failed");

        }

        // Render Pass Creation
        {

            VkAttachmentDescription colorAttachment{};
            colorAttachment.flags = 0;
            colorAttachment.format = c_vlkn::format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = nullptr;
            subpass.pResolveAttachments = nullptr;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkAttachmentDescription attachments[] = { colorAttachment };
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.flags = 0;
            renderPassInfo.attachmentCount = arraySize(attachments);
            renderPassInfo.pAttachments = attachments;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            VkResult err = vkCreateRenderPass(vlkn::device, &renderPassInfo, nullptr, &vlkn::vp::renderPass);
            assert(err == VK_SUCCESS && "Render pass creation failed");

        }

        // Framebuffer creation
        {

            VkImageView attachments[] = { vlkn::vp::imageView };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = vlkn::vp::renderPass;
            framebufferInfo.attachmentCount = arraySize(attachments);
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = c_vlkn::vpWidth;
            framebufferInfo.height = c_vlkn::vpHeight;
            framebufferInfo.layers = 1;

            VkResult err = vkCreateFramebuffer(vlkn::device, &framebufferInfo, nullptr, &vlkn::vp::framebuffer);
            assert(err == VK_SUCCESS && "Frambuffer creation failed");

        }

        // Graphics Pipeline Creation
        {

            VkPipelineShaderStageCreateInfo shaderStages[2]{};

            VkShaderModule vertModule = vlknh::createShaderModule(vlkn::device, "res/shaders/shader.vert.spv");
            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].flags = 0;
            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = vertModule;
            shaderStages[0].pName = "main";

            VkShaderModule fragModule = vlknh::createShaderModule(vlkn::device, "res/shaders/shader.frag.spv");
            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].flags = 0;
            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = fragModule;
            shaderStages[1].pName = "main";

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            VkVertexInputAttributeDescription attribDescriptions[1]{};
            attribDescriptions[0].binding = 0;
            attribDescriptions[0].location = 0;
            attribDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attribDescriptions[0].offset = offsetof(Vertex, pos);

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = arraySize(attribDescriptions);
            vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;

            VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = arraySize(dynamicStates);
            dynamicState.pDynamicStates = dynamicStates;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0;
            pipelineLayoutInfo.pushConstantRangeCount = 0;

            VkResult err = vkCreatePipelineLayout(vlkn::device, &pipelineLayoutInfo, nullptr, &vlkn::vp::pipelineLayout);
            assert(err == VK_SUCCESS && "Failed to create pipeline layout");

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.flags = 0;
            pipelineInfo.stageCount = arraySize(shaderStages);
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pTessellationState = nullptr;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = nullptr;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = vlkn::vp::pipelineLayout;
            pipelineInfo.renderPass = vlkn::vp::renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            err = vkCreateGraphicsPipelines(vlkn::device, nullptr, 1, &pipelineInfo, nullptr, &vlkn::vp::graphicsPipeline);
            assert(err == VK_SUCCESS && "Failed to create graphics pipeline");

            vkDestroyShaderModule(vlkn::device, vertModule, nullptr);
            vkDestroyShaderModule(vlkn::device, fragModule, nullptr);

        }
        {} // Fixes visual studio bug with collapsing scopes

        // Vertex buffer
        {

            VkBuffer       vertStageBuff;
            VkDeviceMemory vertStageMem;

            vlknh::BufferCreateInfo buffInfo{};
            buffInfo.physicalDevice = vlkn::physicalDevice;
            buffInfo.size = sizeof(vertices);
            buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buffInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            vlknh::createBuffer(vlkn::device, buffInfo, &vertStageBuff, &vertStageMem);
            vlknh::loadBuffer(vlkn::device, vertStageMem, vertices, sizeof(vertices));

            buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            buffInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            vlknh::createBuffer(vlkn::device, buffInfo, &vlkn::vp::vertBuff, &vlkn::vp::vertBuffMem);

            vlknh::SingleTimeCommandBuffer cmdBuff(vlkn::device, vlkn::commandPool);
            cmdBuff.begin();
            cmdBuff.copy(sizeof(vertices), vertStageBuff, vlkn::vp::vertBuff);
            cmdBuff.submit(vlkn::graphicsQueue);

            vkQueueWaitIdle(vlkn::graphicsQueue);

            vkFreeMemory(vlkn::device, vertStageMem, nullptr);
            vkDestroyBuffer(vlkn::device, vertStageBuff, nullptr);

        }

        // Texture sampler creation 
        {

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.flags = 0;
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;

            VkResult err = vkCreateSampler(vlkn::device, &samplerInfo, nullptr, &vlkn::vp::frameSampler);
            assert(err == VK_SUCCESS && "Sampler creation failed");

        }

        // framebuffer image -> imgui     
        vlkn::vp::framebufferTexID = ImGui_ImplVulkan_AddTexture(vlkn::vp::frameSampler, vlkn::vp::imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    }

    // All init code goes above this function. 
    ShowWindow(win::hwnd, nCmdShow);

}

static double      frameWaitTime = 0.0; 
constexpr uint32_t sampleCount = 200; 
static float       frameWaitTimesGraph[sampleCount]{}; 
void App::render() {
    
    if (win::windowMinimized) { sleepFor(0.005); return; }

    // Imgui Frame
    {

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

    }
    
    // queue windowButton commands because I can't call the functions while rendering
    bool minimizeWindow = false; 
    bool maximizeWindow = false;
    bool restoreWindow  = false; 
    bool closeWindow    = false; 

    // My GUI
    {
        // Main dockspace
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            windowFlags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            float titleBarPadding = 0.5f * (s_gui.titleBarHeight - s_gui.fontSize);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(0.0f, titleBarPadding));
            ImGui::Begin("Simple Viewer 3D", nullptr, windowFlags);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, titleBarPadding));
            if (ImGui::BeginMenuBar())
            {
                ImGui::SetCursorPosX(s_gui.menuBarStartExtent);
                if (ImGui::BeginMenu("File")) {
                    ImGui::MenuItem("Open");
                    ImGui::MenuItem("Save");
                    ImGui::EndMenu();
                }
                ImGui::SetCursorPosX(0.465f * viewport->Size.x);
                ImGui::Text("Simple Viewer 3D");


                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg)); 
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0, 1.0, 1.0, 0.15));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0, 1.0, 1.0, 0.4));

                float btnWidth     = s_gui.wndBtnWidth;
                float btnHeight    = s_gui.titleBarHeight;
                float iconSize     = 0.25f * btnWidth;  
                float iconStartPos = 0.5f * (btnWidth - iconSize);
                constexpr float iconLineWidth = 2.0;

                // Minimize Button
                ImGui::SetCursorPosX(viewport->Size.x - 3 * btnWidth);
                ImVec2 minimizeIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * btnHeight);
                if (ImGui::Button("##Min", ImVec2(btnWidth, btnHeight))) minimizeWindow = true; 

                // Maximize/Restore Button
                ImGui::SetCursorPosX(viewport->Size.x - 2 * btnWidth);
                ImVec2 windowIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * (btnHeight - iconSize));
                if (ImGui::Button("##Max", ImVec2(btnWidth, btnHeight))) {
                    if (IsZoomed(win::hwnd)) restoreWindow = true; 
                    else maximizeWindow = true;
                }

                // Close App Button 
                ImGui::SetCursorPosX(viewport->Size.x - btnWidth);
                ImVec2 closeIconDrawPos = ImGui::GetCursorPos() + ImVec2(iconStartPos, 0.5f * (btnHeight - iconSize));
                if (ImGui::Button("##Close", ImVec2(btnWidth, btnHeight))) closeWindow = true;

                ImGui::PopStyleColor(3);

                ImDrawList* drawList = ImGui::GetWindowDrawList(); 

                ImU32 iconColor = ImGui::GetColorU32(ImGuiCol_Text); 
                drawList->AddLine(minimizeIconDrawPos, minimizeIconDrawPos + ImVec2(iconSize, 0.0), iconColor, iconLineWidth);
                if (IsZoomed(win::hwnd)) {
                    constexpr float inset = 0.07;
                    ImVec2 recCorner1 = windowIconDrawPos + ImVec2(inset * btnHeight, 0.0);
                    ImVec2 recCorner2 = windowIconDrawPos + ImVec2(iconSize, iconSize - inset * btnHeight);
                    drawList->AddRect(recCorner1, recCorner2, iconColor, 2.0, 0, iconLineWidth);
                    recCorner1 += ImVec2(-iconLineWidth, iconLineWidth);
                    recCorner2 += ImVec2(-iconLineWidth, iconLineWidth);
                    drawList->AddRectFilled(recCorner1, recCorner2, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
                    recCorner1 = windowIconDrawPos + ImVec2(0.0, inset * btnHeight);
                    recCorner2 = windowIconDrawPos + ImVec2(iconSize - inset * btnHeight, iconSize);
                    drawList->AddRect(recCorner1, recCorner2, iconColor, 2.0, 0, iconLineWidth);
                }
                else {
                    drawList->AddRect(windowIconDrawPos, windowIconDrawPos + ImVec2(iconSize, iconSize), iconColor, 2.0, 0, iconLineWidth);
                }


                drawList->AddLine(closeIconDrawPos, closeIconDrawPos + ImVec2(iconSize, iconSize), iconColor, iconLineWidth);
                drawList->AddLine(closeIconDrawPos + ImVec2(0.0, iconSize), closeIconDrawPos + ImVec2(iconSize, 0.0), iconColor, iconLineWidth);


                ImGui::EndMenuBar();
            }
            ImGui::PopStyleVar(2);
            ImGui::End();
            ImGui::PopStyleVar(2);

        }

        ImGui::ShowDemoWindow();

        ImGui::SetNextWindowSize({ 600, 600 }, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("App Data")) {
            ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
            float (*func)(void*, int) = [](void* data, int i) { return frameWaitTimesGraph[i]; };
            constexpr float scale = 0.020;
            ImGui::PlotLines("Frame Time Error", func, nullptr, sampleCount, 0, nullptr, -scale, scale, ImVec2(0, 200));
            ImGui::Text("Scale: %.3fms", scale * 1000);
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin("Viewport")) {

            ImGui::Image(vlkn::vp::framebufferTexID, ImGui::GetContentRegionAvail());

        }
        ImGui::End();
        ImGui::PopStyleVar();

    }
    {} // Fixes visual studio bug with collapsing scopes

    // Rendering
    {

        vkWaitForFences(vlkn::device, 1, &vlkn::frameFinishedFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vlkn::device, 1, &vlkn::frameFinishedFence);

        uint32_t imageIndex;
        VkResult err = vkAcquireNextImageKHR(vlkn::device, vlkn::swapchain, UINT64_MAX, vlkn::imageReadySemaphore, VK_NULL_HANDLE, &imageIndex);
        assert(err == VK_SUCCESS && "Failed to get next swapchain image.");

        vkResetCommandBuffer(vlkn::commandBuff, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        err = vkBeginCommandBuffer(vlkn::commandBuff, &beginInfo);
        assert(err == VK_SUCCESS && "Command Buffer begin failed");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vlkn::vp::renderPass;
        renderPassInfo.framebuffer = vlkn::vp::framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent.width  = c_vlkn::vpWidth;
        renderPassInfo.renderArea.extent.height = c_vlkn::vpHeight;

        VkClearValue clearValues{};
        clearValues.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValues;

        vkCmdBeginRenderPass(vlkn::commandBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(vlkn::commandBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, vlkn::vp::graphicsPipeline); 

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = c_vlkn::vpWidth;
        viewport.height = c_vlkn::vpHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(vlkn::commandBuff, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent.width  = c_vlkn::vpWidth;
        scissor.extent.height = c_vlkn::vpHeight;
        vkCmdSetScissor(vlkn::commandBuff, 0, 1, &scissor);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vlkn::commandBuff, 0, 1, &vlkn::vp::vertBuff, offsets);

        vkCmdDraw(vlkn::commandBuff, 3, 1, 0, 0);

        vkCmdEndRenderPass(vlkn::commandBuff);

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vlkn::renderPass;
        renderPassInfo.framebuffer = vlkn::framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = vlkn::windowImageExtent;

        clearValues.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValues;

        vkCmdBeginRenderPass(vlkn::commandBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vlkn::commandBuff);

        vkCmdEndRenderPass(vlkn::commandBuff);

        err = vkEndCommandBuffer(vlkn::commandBuff);
        assert(err == VK_SUCCESS && "Command buffer end failed");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &vlkn::imageReadySemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vlkn::commandBuff;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &vlkn::renderDoneSemaphore;

        err = vkQueueSubmit(vlkn::graphicsQueue, 1, &submitInfo, vlkn::frameFinishedFence);
        assert(err == VK_SUCCESS && "Queue submit failed");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &vlkn::renderDoneSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vlkn::swapchain;
        presentInfo.pImageIndices = &imageIndex;

        err = vkQueuePresentKHR(vlkn::presentQueue, &presentInfo);
        assert(err == VK_SUCCESS && "Presenting failed");

        for (float* value = frameWaitTimesGraph; value < frameWaitTimesGraph + sampleCount - 1; ++value)
            value[0] = value[1];

        double timeError = win::refreshInterval - ImGui::GetIO().DeltaTime;
        frameWaitTimesGraph[sampleCount - 1] = (float)timeError;
        frameWaitTime += 0.2 * timeError;

        sleepFor(frameWaitTime);

    }

    // Window button commands (In render func for code simplicity)
    {

             if (minimizeWindow) ShowWindow(win::hwnd, SW_MINIMIZE); 
        else if (maximizeWindow) ShowWindow(win::hwnd, SW_MAXIMIZE); 
        else if (restoreWindow)  ShowWindow(win::hwnd, SW_RESTORE);  
        else if (closeWindow)    PostQuitMessage(0);                 

    }

}

void App::close() {

    // IMPORTANT: All vulkan clean up must happen after this line.
    vkDeviceWaitIdle(vlkn::device);
    // IMPORTANT: All vulkan clean up must happen after this line. Leave extra space below.


     // Viewport cleanup
    {

        vkDestroySampler(vlkn::device, vlkn::vp::frameSampler, nullptr);
        vkFreeMemory(vlkn::device, vlkn::vp::vertBuffMem, nullptr);
        vkDestroyBuffer(vlkn::device, vlkn::vp::vertBuff, nullptr);
        vkDestroyPipeline(vlkn::device, vlkn::vp::graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(vlkn::device, vlkn::vp::pipelineLayout, nullptr);
        vkDestroyFramebuffer(vlkn::device, vlkn::vp::framebuffer, nullptr);
        vkDestroyRenderPass(vlkn::device, vlkn::vp::renderPass, nullptr);
        vkDestroyImageView(vlkn::device, vlkn::vp::imageView, nullptr);
        vkFreeMemory(vlkn::device, vlkn::vp::imageMem, nullptr);
        vkDestroyImage(vlkn::device, vlkn::vp::image, nullptr);

    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    vkDestroyFence(vlkn::device, vlkn::frameFinishedFence, nullptr);
    vkDestroySemaphore(vlkn::device, vlkn::renderDoneSemaphore, nullptr);
    vkDestroySemaphore(vlkn::device, vlkn::imageReadySemaphore, nullptr);
    vkDestroyDescriptorPool(vlkn::device, vlkn::descriptorPool, nullptr);
    vkDestroyCommandPool(vlkn::device, vlkn::commandPool, nullptr);

    vkDestroyRenderPass(vlkn::device, vlkn::renderPass, nullptr);
    
    cleanupSwapchainResources(); 
    vkDestroySwapchainKHR(vlkn::device, vlkn::swapchain, nullptr);

    vkDestroyDevice(vlkn::device, nullptr);
    vkDestroySurfaceKHR(vlkn::instance, vlkn::surface, nullptr); 
#ifdef ENABLE_VK_VALIDATION_LAYERS
    DestroyDebugUtilsMessengerEXT(vlkn::instance, vlkn::debugMessenger, nullptr);
#endif 
    vkDestroyInstance(vlkn::instance, nullptr);

} 