#include "App.hpp"
#include "CoreConstants.hpp"

#include <ShellScalingApi.h>
#include <Shlobj.h>

#include <cstdio>
#include <vector>
#include <algorithm>
#include <Timer.hpp>
#include <VulkanHelpers.hpp>

#include <filesystem>
#include <shobjidl.h>
#include <shlguid.h>
#include <atlbase.h>
#include <lmcons.h>

#include <zip.h>
#include <stringLib.h>

#define NANOSVG_IMPLEMENTATION	
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include "FileArrays/Simple_Viewer_3D_zip.h"
#include "FileArrays/Logo_svg.h"

#include <Config.hpp>

/// if (cond == false), the program will terminate and print out errMsg to the console
static void initCheck(bool cond, const char* errMsg) {
#ifdef DEBUG

    if (cond == false) {

        printf(errMsg);
        CORE_ASSERT(!"initCheck Error! Check console output");

    }

#else 

    if (cond == false) {

        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);

        printf(errMsg);
        getchar(); 

        exit(0);

    }

#endif
}

void App::init(Core::Instance* inst, InstanceInfo* initInfo) {

    // Init Variables 
    const char* desiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

    // Console creation
    {

#ifdef ENABLE_VK_VALIDATION_LAYERS
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
#endif

    }

    // Window creation
    {

        SetProcessDPIAware();

        const wchar_t CLASS_NAME[] = L"Main Window";

        WNDCLASS wc = { };

        wc.lpfnWndProc = Core::Callback::WindowProc;
        wc.hInstance = initInfo->hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        HMONITOR mntr = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        UINT dpiX, dpiY;
        GetDpiForMonitor(mntr, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        inst->wind.dpi = float(dpiX / 96.0);

        int mntrWidth  = GetSystemMetrics(SM_CXSCREEN);
        int mntrHeight = GetSystemMetrics(SM_CYSCREEN);

        CORE_ASSERT(c_WindowPercentSize > 0.0 && c_WindowPercentSize <= 1.0);
        CORE_ASSERT(c_WindowAspectRatio > 0.0);

        if (mntrWidth > mntrHeight) {
            inst->wind.size.y = int(c_WindowPercentSize * mntrHeight);
            inst->wind.size.x = int(c_WindowAspectRatio * inst->wind.size.y);
        }
        else {
            inst->wind.size.x = int(c_WindowPercentSize * mntrWidth);
            inst->wind.size.y = int(inst->wind.size.x / c_WindowAspectRatio);
        }

        int startPosX = (mntrWidth  - inst->wind.size.x) / 2;
        int startPosY = (mntrHeight - inst->wind.size.y) / 2;

        inst->wind.hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            L"Simple Viewer 3D Installer",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            startPosX,
            startPosY,
            inst->wind.size.x,
            inst->wind.size.y,
            NULL,
            NULL,
            initInfo->hInstance,
            NULL
        );

        CORE_ASSERT(inst->wind.hwnd != NULL && "Window Creation Failed");

        DEVMODE devMode;
        devMode.dmSize = sizeof(devMode);
        BOOL err = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
        initCheck(err, "EnumDisplaySettings failed");
        inst->wind.refreshInterval = 1.0f / devMode.dmDisplayFrequency; 

    }

    // Vulkan Instance creation
    {

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Simple Viewer 3D installer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

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
            initCheck(false, "Simple Viewer 3D is not supported on your machine. A Vulkan extension does not exist."); 
        extensionExists:;
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = arraySize(extensionNames);
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
            CORE_ASSERT(!"A validation Layer in desiredLayers does not exist.");
        layerExists:;
        }
        createInfo.enabledLayerCount   = arraySize(desiredLayers);
        createInfo.ppEnabledLayerNames = desiredLayers;

        VkDebugUtilsMessengerCreateInfoEXT validationCreateInfo{};
        validationCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        validationCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT /*| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT */ | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        validationCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        validationCreateInfo.pfnUserCallback = Core::Callback::validationLayerCallback;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&validationCreateInfo;
#else
        createInfo.enabledLayerCount   = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext               = nullptr;
#endif

        VkResult err = vkCreateInstance(&createInfo, nullptr, &inst->rend.instance); 
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Vulkan instance could not be created.");

#ifdef ENABLE_VK_VALIDATION_LAYERS

        err = vlknh::createDebugUtilsMessengerEXT(inst->rend.instance, &validationCreateInfo, nullptr, &inst->rend.debugMessenger);
        CORE_ASSERT(err == VK_SUCCESS && "CreateDebugUtilsMessengerEXT failed");

#endif 

    }

    // Vulkan surface creation
    {

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd      = inst->wind.hwnd;
        createInfo.hinstance = initInfo->hInstance;

        VkResult err = vkCreateWin32SurfaceKHR(inst->rend.instance, &createInfo, nullptr, &inst->rend.surface);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Surface creation failed.");

    }
    
    VkPresentModeKHR presentMode; 
    // Pick physical device
    {

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(inst->rend.instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(inst->rend.instance, &deviceCount, physicalDevices.data());

        initCheck(deviceCount > 0, "Simple Viewer 3D is not supported on your machine. No GPUs support Vulkan.");

        std::vector<VkPhysicalDevice> compatableDevices;
        compatableDevices.reserve(deviceCount);

        for (VkPhysicalDevice queriedDevice : physicalDevices) {

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(queriedDevice, &deviceProperties);

            VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
            if (!(counts & c_vlkn::sampleCount)) continue;

            uint32_t queueFamilyPropertyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(queriedDevice, &queueFamilyPropertyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(queriedDevice, &queueFamilyPropertyCount, queueFamilies.data());


            uint32_t index;
            bool hasGraphicsQueue = vlknh::getQueueFamilyFlagsIndex(queriedDevice, VK_QUEUE_GRAPHICS_BIT, &index);
            bool hasPresentQueue  = vlknh::getQueueFamilyPresentIndex(queriedDevice, inst->rend.surface, &index);
            if (!hasGraphicsQueue || !hasPresentQueue) continue;

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
            if (!hasSwapchain) continue;

            bool formatsAdequate = false;

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(queriedDevice, inst->rend.surface, &formatCount, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(queriedDevice, inst->rend.surface, &formatCount, formats.data());
            for (const VkSurfaceFormatKHR& format : formats) {
                if (format.format == c_vlkn::format && format.colorSpace == c_vlkn::colorSpace) {
                    formatsAdequate = true;
                    break;
                }
            }
            if (!formatsAdequate) continue;

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(queriedDevice, inst->rend.surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(queriedDevice, inst->rend.surface, &presentModeCount, presentModes.data());
            auto hasPresentMode = [](const std::vector<VkPresentModeKHR>& modes, VkPresentModeKHR desiredMode) {
                for (VkPresentModeKHR presentMode : modes) {
                    if (presentMode == desiredMode) return true;
                }
                return false;
                };
            if      (hasPresentMode(presentModes, c_vlkn::presentMode))       presentMode = c_vlkn::presentMode;
            else if (hasPresentMode(presentModes, c_vlkn::backupPresentMode)) presentMode = c_vlkn::backupPresentMode;
            else { continue; }

            compatableDevices.push_back(queriedDevice);
        }

        initCheck(deviceCount > 0, "Simple Viewer 3D is not supported on your machine. No GPUs are compatable with app.");

        inst->rend.physicalDevice = compatableDevices[0];
        for (VkPhysicalDevice compatableDevice : compatableDevices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(compatableDevice, &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                inst->rend.physicalDevice = compatableDevice;
                break;
            }
        }

    }

    // Logical device creation
    {

        vlknh::getQueueFamilyFlagsIndex   (inst->rend.physicalDevice, VK_QUEUE_GRAPHICS_BIT, &inst->rend.graphicsQueueIndex);
        vlknh::getQueueFamilyPresentIndex (inst->rend.physicalDevice, inst->rend.surface,    &inst->rend.presentQueueIndex);

        VkDeviceQueueCreateInfo queueCreateInfos[2]{};

        float queuePriority = 1.0;
        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].queueFamilyIndex = inst->rend.graphicsQueueIndex;
        queueCreateInfos[0].queueCount       = 1;
        queueCreateInfos[0].pQueuePriorities = &queuePriority;

        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].queueFamilyIndex = inst->rend.presentQueueIndex;
        queueCreateInfos[1].queueCount       = 1;
        queueCreateInfos[1].pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos    = queueCreateInfos;
        createInfo.queueCreateInfoCount = arraySize(queueCreateInfos);
        if (inst->rend.graphicsQueueIndex == inst->rend.presentQueueIndex) createInfo.queueCreateInfoCount -= 1;

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

        VkResult err = vkCreateDevice(inst->rend.physicalDevice, &createInfo, nullptr, &inst->rend.device);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Vulkan logical device could not be created.");

    }

    // Queue retrieval
    {

        vkGetDeviceQueue(inst->rend.device, inst->rend.graphicsQueueIndex, 0, &inst->rend.graphicsQueue);
        vkGetDeviceQueue(inst->rend.device, inst->rend.presentQueueIndex,  0, &inst->rend.presentQueue);

    }

    // Swapchain creation 
    {

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.flags   = 0;
        createInfo.surface = inst->rend.surface;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(inst->rend.physicalDevice, inst->rend.surface, &capabilities);

        initCheck(c_vlkn::imageCount >= capabilities.minImageCount, "Simple Viewer 3D is not supported on your machine. Swapchain doesn't have high enough image count.");

        createInfo.minImageCount   = c_vlkn::imageCount;
        createInfo.imageFormat     = c_vlkn::format;
        createInfo.imageColorSpace = c_vlkn::colorSpace;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            createInfo.imageExtent = capabilities.currentExtent;
        }
        else {
            createInfo.imageExtent = {
                std::clamp((unsigned)inst->wind.size.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp((unsigned)inst->wind.size.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };

        }

        inst->rend.windowImageExtent = createInfo.imageExtent;

        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { inst->rend.graphicsQueueIndex, inst->rend.presentQueueIndex };
        if (inst->rend.graphicsQueueIndex == inst->rend.presentQueueIndex) {
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
        }
        else {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
        }
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        createInfo.preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = VK_TRUE;
        createInfo.oldSwapchain   = VK_NULL_HANDLE;

        VkResult err = vkCreateSwapchainKHR(inst->rend.device, &createInfo, nullptr, &inst->rend.swapchain);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Swapchain creation failed.");

    }

    // Swap chain images
    {

        vkGetSwapchainImagesKHR(inst->rend.device, inst->rend.swapchain, &inst->rend.imageCount, nullptr);
        inst->rend.swapchainImages = new VkImage[inst->rend.imageCount];
        vkGetSwapchainImagesKHR(inst->rend.device, inst->rend.swapchain, &inst->rend.imageCount, inst->rend.swapchainImages);

    }

    // Swap chain image views
    {

        inst->rend.swapchainImageViews = new VkImageView[inst->rend.imageCount];
        for (size_t i = 0; i < inst->rend.imageCount; ++i) {

            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.flags    = 0;
            createInfo.image    = inst->rend.swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format   = c_vlkn::format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &inst->rend.swapchainImageViews[i]);
            initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Image view creation failed.");

        }

    }

    // Render pass creation 
    {

        VkAttachmentDescription colorAttachment{};
        colorAttachment.flags = 0;
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
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.pResolveAttachments     = nullptr;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

        VkResult err = vkCreateRenderPass(inst->rend.device, &renderPassInfo, nullptr, &inst->rend.renderPass);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Render pass creation failed.");

    }

    // Framebuffers creation 
    {

        inst->rend.framebuffers = new VkFramebuffer[inst->rend.imageCount];

        for (uint32_t i = 0; i < inst->rend.imageCount; ++i) {

            VkImageView attachments[] = { inst->rend.swapchainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = inst->rend.renderPass;
            framebufferInfo.attachmentCount = arraySize(attachments);
            framebufferInfo.pAttachments    = attachments;
            framebufferInfo.width           = inst->rend.windowImageExtent.width;
            framebufferInfo.height          = inst->rend.windowImageExtent.height;
            framebufferInfo.layers          = 1;

            VkResult err = vkCreateFramebuffer(inst->rend.device, &framebufferInfo, nullptr, &inst->rend.framebuffers[i]);
            initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Frambuffer creation failed.");

        }

    }

    // Command Pool Creation 
    {

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = inst->rend.graphicsQueueIndex;

        VkResult err = vkCreateCommandPool(inst->rend.device, &createInfo, nullptr, &inst->rend.commandPool);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Command pool creation failed.");

    }

    // Command buffer creation
    {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = inst->rend.commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult err = vkAllocateCommandBuffers(inst->rend.device, &allocInfo, &inst->rend.commandBuff);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Command buffer allocation failed.");

    }

    // Decriptor pool creation 
    {

        VkDescriptorPoolSize poolSizeInfo{};
        poolSizeInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizeInfo.descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets       = 2;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSizeInfo;

        VkResult err = vkCreateDescriptorPool(inst->rend.device, &poolInfo, nullptr, &inst->rend.descriptorPool);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Descriptor pool creation failed.");

    }
    
    // Texture sampler creation 
    {

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.flags            = 0;
        samplerInfo.magFilter        = VK_FILTER_NEAREST;
        samplerInfo.minFilter        = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipLodBias       = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy    = 1.0f;
        samplerInfo.compareEnable    = VK_FALSE;
        samplerInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod           = 0.0f;
        samplerInfo.maxLod           = 0.0f;
        samplerInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        VkResult err = vkCreateSampler(inst->rend.device, &samplerInfo, nullptr, &inst->rend.logoSampler);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Sampler creation failed.");

    }

    // Descriptor Set Layout creation
    {

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding            = 0;
        samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount    = 1;
        samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.flags        = 0;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &samplerLayoutBinding;

        VkResult err = vkCreateDescriptorSetLayout(inst->rend.device, &layoutInfo, nullptr, &inst->rend.descriptorSetLayout);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Descriptor set layout creation failed.");

    }

    // Semaphore and Fence creation 
    {

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult err1 = vkCreateSemaphore(inst->rend.device, &semaphoreInfo, nullptr, &inst->rend.imageReadySemaphore);
        VkResult err2 = vkCreateSemaphore(inst->rend.device, &semaphoreInfo, nullptr, &inst->rend.renderDoneSemaphore);
        initCheck(err1 == VK_SUCCESS || err2 == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Semaphore creation failed.");


        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult err = vkCreateFence(inst->rend.device, &fenceInfo, nullptr, &inst->rend.frameFinishedFence);
        initCheck(err == VK_SUCCESS, "Simple Viewer 3D is not supported on your machine. Fence creation failed.");

    }

    // GUI initialization
    {

        Gui::InitInfo guiInitInfo{};
        guiInitInfo.hwnd               = inst->wind.hwnd;
        guiInitInfo.instance           = inst->rend.instance;
        guiInitInfo.physicalDevice     = inst->rend.physicalDevice;
        guiInitInfo.device             = inst->rend.device;
        guiInitInfo.graphicsQueueIndex = inst->rend.graphicsQueueIndex;
        guiInitInfo.graphicsQueue      = inst->rend.graphicsQueue;
        guiInitInfo.descriptorPool     = inst->rend.descriptorPool;
        guiInitInfo.renderPass         = inst->rend.renderPass;
        guiInitInfo.subpass            = 0;
        guiInitInfo.imageCount         = inst->rend.imageCount;

        Gui::init(&guiInitInfo, &inst->gui.guiSizes, inst->wind.dpi);

        NSVGimage* image = nsvgParse((char*)Logo_svg, "px", 96); 
        assert(image != nullptr && "Failed to parse Logo_svg");

        struct NSVGrasterizer* rast = nsvgCreateRasterizer();
        inst->gui.logoSize.x = floorf(0.1f * inst->wind.size.x);
        inst->gui.logoSize.y = inst->gui.logoSize.x;
        int width            = (int)inst->gui.logoSize.x;
        int height           = (int)inst->gui.logoSize.y;
        unsigned char* logoImg = (unsigned char*)malloc(width * height * 4);
        nsvgRasterize(rast, image, 0, 0, width / 100.0f, logoImg, width, height, width * 4);

        nsvgDeleteRasterizer(rast);

        vlknh::TextureImageCreateInfo texImageInfo{};
        texImageInfo.physicalDevice      = inst->rend.physicalDevice;
        texImageInfo.commandPool         = inst->rend.commandPool;
        texImageInfo.graphicsQueue       = inst->rend.graphicsQueue;
        texImageInfo.descriptorPool      = inst->rend.descriptorPool;
        texImageInfo.descriptorSetLayout = inst->rend.descriptorSetLayout; 
        texImageInfo.sampler             = inst->rend.logoSampler;
        texImageInfo.imageSize           = { (uint32_t)width, (uint32_t)height };
        texImageInfo.pPixelData          = logoImg;

        vlknh::TextureImageResources texImgResources{};
        texImgResources.pTexImg     = &inst->rend.logoImg;
        texImgResources.pTexImgMem  = &inst->rend.logoImgMem;
        texImgResources.pTexImgView = &inst->rend.logoImgView;

        inst->gui.logoTexId = (ImTextureID)vlknh::createTextureImage(inst->rend.device, texImageInfo, texImgResources); 

        free(image);
        free(logoImg); 

    }

    // All init code goes above these functions. 
    ShowWindow(inst->wind.hwnd, initInfo->nCmdShow);

}

void App::render(Core::Instance* inst) {

    Gui::draw(&inst->gui); 
    
    // Following if statement is 90% chatgpt. No shot I'm spending the time to learn all of this. 
    if (inst->gui.openFolderDialog) {
        inst->gui.openFolderDialog = false;

        CoInitialize(nullptr);

        IFileDialog* pFileDialog = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));

        if (SUCCEEDED(hr)) {
            DWORD options;
            pFileDialog->GetOptions(&options);
            pFileDialog->SetOptions(options | FOS_PICKFOLDERS); 

            pFileDialog->SetTitle(L"Select Folder");

            hr = pFileDialog->Show(inst->wind.hwnd);

            if (SUCCEEDED(hr)) {
                IShellItem* pItem;
                hr = pFileDialog->GetResult(&pItem);

                if (SUCCEEDED(hr)) {
                    PWSTR folderPath = nullptr;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &folderPath);

                    if (SUCCEEDED(hr)) {
                        CoTaskMemFree(folderPath); 
                        pItem->Release();
                        pFileDialog->Release();

                        inst->installDir = folderPath;
                        const char* folderPathA = wcharToChar(folderPath);
                        inst->gui.guiInstallDir = folderPathA; 
                        inst->gui.guiInstallDir += "\\Simple Viewer 3D";
                        free((void*)folderPathA); 
                        goto exit; 
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }

    exit: 
        CoUninitialize();
    }

    vkWaitForFences(inst->rend.device, 1, &inst->rend.frameFinishedFence, VK_TRUE, UINT64_MAX);
    vkResetFences(inst->rend.device, 1, &inst->rend.frameFinishedFence);

    uint32_t imageIndex;
    VkResult err = vkAcquireNextImageKHR(inst->rend.device, inst->rend.swapchain, UINT64_MAX, inst->rend.imageReadySemaphore, VK_NULL_HANDLE, &imageIndex);
    CORE_ASSERT(err == VK_SUCCESS && "Failed to get next swapchain image.");

    vkResetCommandBuffer(inst->rend.commandBuff, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    err = vkBeginCommandBuffer(inst->rend.commandBuff, &beginInfo);
    CORE_ASSERT(err == VK_SUCCESS && "Command Buffer begin failed");

    VkClearValue clearValue{};
    clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f } };
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = inst->rend.renderPass;
    renderPassInfo.framebuffer       = inst->rend.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = inst->rend.windowImageExtent;

    clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    

    vkCmdBeginRenderPass(inst->rend.commandBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); 

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inst->rend.commandBuff);

    vkCmdEndRenderPass(inst->rend.commandBuff);

    err = vkEndCommandBuffer(inst->rend.commandBuff);
    CORE_ASSERT(err == VK_SUCCESS && "Command buffer end failed");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &inst->rend.imageReadySemaphore;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &inst->rend.commandBuff;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &inst->rend.renderDoneSemaphore;

    err = vkQueueSubmit(inst->rend.graphicsQueue, 1, &submitInfo, inst->rend.frameFinishedFence);
    CORE_ASSERT(err == VK_SUCCESS && "Queue submit failed");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &inst->rend.renderDoneSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &inst->rend.swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    err = vkQueuePresentKHR(inst->rend.presentQueue, &presentInfo);
    CORE_ASSERT(err == VK_SUCCESS && "Presenting failed");

    float timeError = inst->wind.refreshInterval - ImGui::GetIO().DeltaTime;
    inst->rend.frameWaitTime += 0.5f * timeError; 
    sleepFor(inst->rend.frameWaitTime); 

}

void App::install(Core::Instance* inst) {
    inst->gui.install = false;

    void (*installFunc)(Core::Instance* inst) = [](Core::Instance* inst){

        wchar_t installDrive[4] = L"C:\\";
        installDrive[0] = inst->installDir[0];
        ULARGE_INTEGER freeBytes; 
        GetDiskFreeSpaceEx(installDrive, nullptr, nullptr, &freeBytes); 
        if (freeBytes.QuadPart < cfg::approxAppSize) { inst->gui.currentPage = Gui::guiPage_NotEnoughSpace; return; }

        char* installDirA = wcharToChar(inst->installDir.c_str()); 

        zip_stream_extract((const char*)Simple_Viewer_3D_zip, sizeof(Simple_Viewer_3D_zip), installDirA, nullptr, nullptr);

        const wchar_t* installFolderStrs[] = { inst->installDir.c_str(), L"\\", cfg::appName };
        const wchar_t* installFolder = concatMultipleW(installFolderStrs, arraySize(installFolderStrs));
        const wchar_t* exePathStrs[] = { installFolder, L"\\", cfg::appName, L".exe" };
        const wchar_t* exePath = concatMultipleW(exePathStrs, arraySize(exePathStrs)); 

        // Filesystem management
        {

            HRESULT hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
            assert(hResult == S_OK && "CoInitializeEx failed!");

            CComPtr<IShellLink> shellLink;
            hResult = shellLink.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
            assert(hResult == S_OK && "CoCreateInstance failed!");


            shellLink->SetPath(exePath);
            shellLink->SetWorkingDirectory(installFolder);
            const wchar_t* description = concatW(cfg::appName, L" shortcut"); 
            shellLink->SetDescription(description);

            CComPtr<IPersistFile> persistFile;
            hResult = shellLink.QueryInterface(&persistFile);
            assert(hResult == S_OK && "QueryInterface failed");

            const wchar_t* startMenuDir = concatW(L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\", cfg::appName);
            CreateDirectory(startMenuDir, NULL);
            
            const wchar_t* startMenuLnkStrs[] = { startMenuDir, L"\\", cfg::appName, L".lnk" };
            const wchar_t* startMenuLnk = concatMultipleW(startMenuLnkStrs, arraySize(startMenuLnkStrs));
            persistFile->Save(startMenuLnk, TRUE);

            if (inst->gui.desktopShortcut) {

                wchar_t username[UNLEN + 1]; 
                DWORD   usernameLen = UNLEN + 1;

                BOOL err = GetUserName(username, &usernameLen); 
                assert(err != 0 && "Failed to Get username!");
                const wchar_t* desktopShortcutCloudPathStrs[] = { L"C:\\Users\\", username, L"\\OneDrive\\Desktop\\", cfg::appName, L".lnk" };
                const wchar_t* desktopShortcutCloudPath = concatMultipleW(desktopShortcutCloudPathStrs, arraySize(desktopShortcutCloudPathStrs));

                persistFile->Save(desktopShortcutCloudPath, TRUE);

                const wchar_t* desktopShortcutLocalPathStrs[] = { L"C:\\Users\\", username, L"\\Desktop\\", cfg::appName, L".lnk" };
                const wchar_t* desktopShortcutLocalPath = concatMultipleW(desktopShortcutLocalPathStrs, arraySize(desktopShortcutLocalPathStrs));

                persistFile->Save(desktopShortcutLocalPath, TRUE);

                free((void*)desktopShortcutCloudPath);
                free((void*)desktopShortcutLocalPath);

            }

            CoUninitialize();

            free((void*)description);
            free((void*)startMenuDir);
            free((void*)startMenuLnk);
            
        }

        // Registry edits
        {

            const wchar_t* regKeyPath = concatW(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\", cfg::appNameNoSpaces);
            Core::RegKey uinstallData(HKEY_LOCAL_MACHINE, regKeyPath);

            uinstallData.set(L"InstallLocation", installFolder); 
            uinstallData.set(L"DisplayName", cfg::appName);
            uinstallData.set(L"DisplayVersion", cfg::version);
            uinstallData.set(L"DisplayIcon", exePath);
            constexpr DWORD estimatedUninstallSize = cfg::approxAppSize / (DWORD)1e3; // convert to Megabytes
            static_assert(estimatedUninstallSize >= 1); 
            uinstallData.set(L"EstimatedSize", estimatedUninstallSize); 

            const wchar_t* uninstallPath = concatW(installFolder, L"\\Uninstall.exe");
            uinstallData.set(L"UninstallString", uninstallPath); 

            // File association related registry edits... 
            const wchar_t* applicationKeyStrs[] = { L"Applications\\", cfg::launcherName, L".exe"};
            const wchar_t* applicationKeyStr = concatMultipleW(applicationKeyStrs, arraySize(applicationKeyStrs)); 
            Core::RegKey applicationKey(HKEY_CLASSES_ROOT, applicationKeyStr);
            applicationKey.set(L"FriendlyAppName", L"Simple Viewer 3D");

            const wchar_t* applicationOpenKeyStr = concatW(applicationKeyStr, L"\\shell\\open\\command");
            Core::RegKey applicationOpenKey(HKEY_CLASSES_ROOT, applicationOpenKeyStr);
            const wchar_t* shellCommandStrs[] = { L"\"", installFolder, L"\\", cfg::launcherName, L".exe\" \"", installFolder, L"\" \"%1\""};
            const wchar_t* shellCommand = concatMultipleW(shellCommandStrs, arraySize(shellCommandStrs));
            applicationOpenKey.set(L"", shellCommand);

            Core::RegKey sltFileAssociationKey(HKEY_CLASSES_ROOT, L".stl\\OpenWithProgids"); 
            const wchar_t* fileClass = concatW(cfg::appNameNoSpaces, L"_file");
            sltFileAssociationKey.set(fileClass, L"");

            Core::RegKey objFileAssociationKey(HKEY_CLASSES_ROOT, L".obj\\OpenWithProgids");
            objFileAssociationKey.set(fileClass, L"");

            const wchar_t* fileAssociationClassOpenKeyStr = concatW(fileClass, L"\\shell\\open\\command");
            Core::RegKey fileAssociationClassOpenKey(HKEY_CLASSES_ROOT, fileAssociationClassOpenKeyStr);
            fileAssociationClassOpenKey.set(L"", shellCommand); 

            free((void*)regKeyPath); 
            free((void*)uninstallPath);
            free((void*)applicationKeyStr);
            free((void*)applicationOpenKeyStr);
            free((void*)shellCommand); 
            free((void*)fileClass); 
            free((void*)fileAssociationClassOpenKeyStr);

        }

        
        free(installDirA);
        free((void*)installFolder);
        free((void*)exePath);

        // Tell gui to go to the next page
        Gui::incr(inst->gui.currentPage); 
    };

    std::thread installThread(installFunc, inst); 
    installThread.detach();

}

void App::close(Core::Instance* inst) {

    // IMPORTANT: All vulkan clean up must happen after this line.
    vkDeviceWaitIdle(inst->rend.device);
    // IMPORTANT: All vulkan clean up must happen after this line. Leave extra space below.

    Gui::destroy();

    vkDestroyImageView           (inst->rend.device, inst->rend.logoImgView,         nullptr);
    vkDestroyImage               (inst->rend.device, inst->rend.logoImg,             nullptr);
    vkFreeMemory                 (inst->rend.device, inst->rend.logoImgMem,          nullptr);
    vkDestroyDescriptorSetLayout (inst->rend.device, inst->rend.descriptorSetLayout, nullptr);
    vkDestroySampler             (inst->rend.device, inst->rend.logoSampler,         nullptr);
    vkDestroyFence               (inst->rend.device, inst->rend.frameFinishedFence,  nullptr);
    vkDestroySemaphore           (inst->rend.device, inst->rend.renderDoneSemaphore, nullptr);
    vkDestroySemaphore           (inst->rend.device, inst->rend.imageReadySemaphore, nullptr);
    vkDestroyDescriptorPool      (inst->rend.device, inst->rend.descriptorPool,      nullptr);
    vkDestroyCommandPool         (inst->rend.device, inst->rend.commandPool,         nullptr);
    vkDestroyRenderPass          (inst->rend.device, inst->rend.renderPass,          nullptr);

    for (unsigned i = 0; i < inst->rend.imageCount; ++i) {
        vkDestroyFramebuffer(inst->rend.device, inst->rend.framebuffers[i], nullptr);
    }
    delete[] inst->rend.framebuffers;
    for (unsigned i = 0; i < inst->rend.imageCount; ++i) {
        vkDestroyImageView(inst->rend.device, inst->rend.swapchainImageViews[i], nullptr);
    }
    delete[] inst->rend.swapchainImageViews;
    delete[] inst->rend.swapchainImages;
    vkDestroySwapchainKHR(inst->rend.device, inst->rend.swapchain, nullptr);

    vkDestroyDevice(inst->rend.device, nullptr);
    vkDestroySurfaceKHR(inst->rend.instance, inst->rend.surface, nullptr);

#ifdef ENABLE_VK_VALIDATION_LAYERS
    vlknh::destroyDebugUtilsMessengerEXT(inst->rend.instance, inst->rend.debugMessenger, nullptr);
#endif 
    vkDestroyInstance(inst->rend.instance, nullptr);

    if (inst->gui.launchOnFinish && inst->gui.currentPage == Gui::guiPage_finish) {
        PROCESS_INFORMATION processInfo{};

        // Set up the startup information structure
        STARTUPINFO startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        const wchar_t* exePath    = concatW(inst->installDir.c_str(), L"\\Simple Viewer 3D\\Simple Viewer 3D.exe");
        const wchar_t* workingDir = concatW(inst->installDir.c_str(), L"\\Simple Viewer 3D");
        // Create the process
        CreateProcess(
            exePath,
            NULL,
            NULL,
            NULL,
            FALSE,
            DETACHED_PROCESS,
            NULL,
            workingDir,
            &startupInfo,
            &processInfo
        );
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

        free((void*)exePath);
        free((void*)workingDir); 

    }
}