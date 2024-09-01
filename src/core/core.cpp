

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>

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

// static globals (didn't mark static because for less verbose syntax)
namespace win {

    int  windowWidth, windowHeight; 
    HWND hwnd;
    double refreshInterval = 1.0 / 120; // 1 / hz   hz = monitor refresh rate

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
    VkCommandPool            commandPool; 
    VkCommandBuffer          commandBuff; 
    VkDescriptorPool         descriptorPool;
    VkSemaphore              renderDoneSemaphore;
    VkSemaphore              imageReadySemaphore;
    VkFence                  frameFinishedFence;

}

glm::vec3 vertices[] = {
    {  0.0f, -0.5f,  0.0f },
    {  0.5f,  0.5f,  0.0f },
    { -0.5f,  0.5f,  0.0f },
};

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

        // Getting the window scaling
        // HMONITOR mntr = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        // UINT dpiX, dpiY;
        // GetDpiForMonitor(mntr, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        // float scale = float(dpiX / 96.0);

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

    // Swapchain creation 
    {

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.flags   = 0;
        createInfo.surface = vlkn::surface;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vlkn::physicalDevice, vlkn::surface, &capabilities);

        assert(c_vlkn::imageCount >= capabilities.minImageCount);

        createInfo.minImageCount   = c_vlkn::imageCount;
        createInfo.imageFormat     = c_vlkn::format;
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
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
        }
        else {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
        }
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        createInfo.preTransform        = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha      = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode         = c_vlkn::presentMode;
        createInfo.clipped             = VK_TRUE;
        createInfo.oldSwapchain        = VK_NULL_HANDLE;

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
            createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.flags        = 0;
            createInfo.image        = vlkn::swapchainImages[i];
            createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format       = c_vlkn::format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            VkResult err = vkCreateImageView(vlkn::device, &createInfo, nullptr, &vlkn::swapchainImageViews[i]);
            assert(err == VK_SUCCESS && "Image view creation failed");

        }

    }

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

    // Framebuffer creation
    {
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

    // Command Pool Creation 
    {

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = vlkn::graphicsQueueIndex;

        VkResult err = vkCreateCommandPool(vlkn::device, &createInfo, nullptr, &vlkn::commandPool);
        assert(err == VK_SUCCESS && "Command pool creation failed");

    }

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
        poolInfo.maxSets       = 1;
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


        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 28.0f);
        io.Fonts->Build();

    }

    // All init code goes above this function. 
    ShowWindow(win::hwnd, nCmdShow);

}

static double      frameWaitTime = 0.0; 
constexpr uint32_t sampleCount = 200; 
static float       frameWaitTimesGraph[sampleCount]{}; 
void App::render() {

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    ImGui::SetNextWindowSize({600, 600}, ImGuiCond_FirstUseEver);

    ImGuiIO& io = ImGui::GetIO(); 
    if ( ImGui::Begin("App Data") ) {
        ImGui::Text("Framerate: %f", io.Framerate); 
        auto func = [](void* data, int i) { return frameWaitTimesGraph[i]; };
        constexpr float scale = 0.020; 
        ImGui::PlotLines("Frame Time Error", func, nullptr, sampleCount, 0, nullptr, -scale, scale, ImVec2(0, 200));
        ImGui::Text("Scale: %.3fms", scale * 1000);
        ImGui::End(); 
    }

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
    renderPassInfo.renderPass = vlkn::renderPass;
    renderPassInfo.framebuffer = vlkn::framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vlkn::windowImageExtent;

    VkClearValue clearValues{}; 
    clearValues.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearValues;

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

    double timeError = win::refreshInterval - io.DeltaTime; 
    frameWaitTimesGraph[sampleCount - 1] = (float)timeError; 
    frameWaitTime += std::clamp(timeError, -0.005, 0.005);

    sleepFor(frameWaitTime); 

}

void App::close() {
    // IMPORTANT: All vulkan clean up must happen after this line.
    vkDeviceWaitIdle(vlkn::device);
    // IMPORTANT: All vulkan clean up must happen after this line. Leave extra space below.




    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    vkDestroyFence(vlkn::device, vlkn::frameFinishedFence, nullptr);
    vkDestroySemaphore(vlkn::device, vlkn::renderDoneSemaphore, nullptr);
    vkDestroySemaphore(vlkn::device, vlkn::imageReadySemaphore, nullptr);
    vkDestroyDescriptorPool(vlkn::device, vlkn::descriptorPool, nullptr);
    vkDestroyCommandPool(vlkn::device, vlkn::commandPool, nullptr);
    for (unsigned i = 0; i < vlkn::imageCount; ++i) {
        vkDestroyFramebuffer(vlkn::device, vlkn::framebuffers[i], nullptr);
    }
    delete[] vlkn::framebuffers;
    vkDestroyRenderPass(vlkn::device, vlkn::renderPass, nullptr);
    for (unsigned i = 0; i < vlkn::imageCount; ++i) {
        vkDestroyImageView(vlkn::device, vlkn::swapchainImageViews[i], nullptr);
    }
    delete[] vlkn::swapchainImageViews;
    delete[] vlkn::swapchainImages;
    vkDestroySwapchainKHR(vlkn::device, vlkn::swapchain, nullptr);
    vkDestroyDevice(vlkn::device, nullptr);
    vkDestroySurfaceKHR(vlkn::instance, vlkn::surface, nullptr); 
#ifdef ENABLE_VK_VALIDATION_LAYERS
    DestroyDebugUtilsMessengerEXT(vlkn::instance, vlkn::debugMessenger, nullptr);
#endif 
    vkDestroyInstance(vlkn::instance, nullptr);

} 