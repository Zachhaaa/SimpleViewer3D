#include "App.hpp"
#include "CoreConstants.hpp"
#include "CustomIniData.hpp"

#include <ShellScalingApi.h>

#include <VulkanHelpers.hpp>

#include "FileArrays/shader_vert_spv.h"
#include "FileArrays/shader_frag_spv.h"
#include "FileArrays/MeshOutlineShader_vert_spv.h"
#include "FileArrays/MeshOutlineShader_frag_spv.h"

#include <ModelLoader.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#define NANOSVG_IMPLEMENTATION	
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include <shlobj.h>

struct PushConstants {
    glm::mat4 view, proj; 
};
#ifdef DEBUG
#define assertExit(cond, msg) CORE_ASSERT(cond && msg); 
#else 
#define assertExit(cond, msg) if (!(cond)) exit(1); 
#endif

void App::init(Core::Instance* inst, const InstanceInfo& initInfo) {

    scopedTimer(t1, inst->gui.stats.perfTimes.getTimer("appLaunch"));
    
    // Init Variables 
    const char* desiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

    // Console creation
    {

#ifdef ENABLE_VK_VALIDATION_LAYERS
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
#endif
        
    }

    bool maximized = false; 
    const char* iniRelativePath = "\\Simple Viewer 3D\\imgui.ini";
    char* iniPath = (char*)malloc(MAX_PATH + strlen(iniRelativePath) + 1);
    CORE_ASSERT(iniPath != nullptr); 

    // Window creation
    {

        SetProcessDPIAware();

        const wchar_t CLASS_NAME[] = L"Main Window";

        WNDCLASS wc{};
        wc.lpfnWndProc = Core::Callback::WindowProc;
        wc.hInstance = initInfo.hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        HMONITOR mntr = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        UINT dpiX, dpiY;
        GetDpiForMonitor(mntr, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        inst->wind.dpi = float(dpiX / 96.0);
        
        int startPosX; 
        int startPosY; 
        
        // Get imgui.ini path
        {

            SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, iniPath);
            char *endC = iniPath; 
            for (; *endC != '\0'; endC++) {}
            strcpy(endC, "\\Simple Viewer 3D");
            CreateDirectoryA(iniPath, NULL); 
            endC = iniPath;
            for (; *endC != '\0'; endC++) {}
            strcpy(endC, "\\imgui.ini"); 

        }

        CustomIniData iniData{};
        inst->gui.sensitivity = 50; 
        bool dataExists = getCustomIniData(&iniData, iniPath);
        if (dataExists && (iniData.windowWidth != 0 && iniData.windowHeight != 0)) {

            inst->gui.sensitivity = (float)iniData.sensitivity;
            startPosX             = iniData.windowPosX;
            startPosY             = iniData.windowPosY;
            inst->wind.m_size.x   = iniData.windowWidth; 
            inst->wind.m_size.y   = iniData.windowHeight; 
            maximized             = iniData.windowMaximized;  

        }
        else {

            int mntrWidth  = GetSystemMetrics(SM_CXSCREEN);
            int mntrHeight = GetSystemMetrics(SM_CYSCREEN);

            CORE_ASSERT(c_WindowPercentSize > 0.0 && c_WindowPercentSize <= 1.0);
            CORE_ASSERT(c_WindowAspectRatio > 0.0);

            if (mntrWidth > mntrHeight) {
                inst->wind.m_size.y = int(c_WindowPercentSize * mntrHeight);
                inst->wind.m_size.x = int(c_WindowAspectRatio * inst->wind.m_size.y);
            }
            else {
                inst->wind.m_size.x = int(c_WindowPercentSize * mntrWidth);
                inst->wind.m_size.y = int(inst->wind.m_size.x / c_WindowAspectRatio);
            }

            startPosX = int(0.5 * (mntrWidth  - inst->wind.m_size.x));
            startPosY = int(0.5 * (mntrHeight - inst->wind.m_size.y));
          
        }

        inst->wind.hwnd = CreateWindowEx(
            WS_EX_ACCEPTFILES,
            CLASS_NAME,
            L"Simple Viewer 3D",
            WS_OVERLAPPEDWINDOW,
            startPosX,
            startPosY,
            inst->wind.m_size.x,
            inst->wind.m_size.y,
            NULL,
            NULL,
            initInfo.hInstance,
            NULL
        );

        assertExit(inst->wind.hwnd != NULL, "Window Creation Failed");

        SetWindowLongPtr(inst->wind.hwnd, GWLP_USERDATA, (LONG_PTR)inst);

        DEVMODE devMode;
        devMode.dmSize = sizeof(devMode);
        BOOL err = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
        assertExit(err, "EnumDisplaySettings failed");
        inst->wind.refreshInterval = 1.0f / devMode.dmDisplayFrequency;

    }

    // Vulkan Instance creation 
    {

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Simple Viewer 3D";
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
            assertExit(0, "An extension in entensionNames does not exist.");
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
            assertExit(0, "A validation Layer in desiredLayers does not exist.");
        layerExists:;
        }
        createInfo.enabledLayerCount = arraySize(desiredLayers);
        createInfo.ppEnabledLayerNames = desiredLayers;

        VkDebugUtilsMessengerCreateInfoEXT validationCreateInfo{};
        validationCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        validationCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT /*| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT */ | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        validationCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        validationCreateInfo.pfnUserCallback = Core::Callback::validationLayerCallback;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&validationCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
#endif

        VkResult err = vkCreateInstance(&createInfo, nullptr, &inst->rend.instance);
        assertExit(err == VK_SUCCESS,"Vulkan instance creation failed");

#ifdef ENABLE_VK_VALIDATION_LAYERS

        err = vlknh::createDebugUtilsMessengerEXT(inst->rend.instance, &validationCreateInfo, nullptr, &inst->rend.debugMessenger);
        assertExit(err == VK_SUCCESS, "CreateDebugUtilsMessengerEXT failed");

#endif 
    }

    // Vulkan surface creation
    {

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = inst->wind.hwnd;
        createInfo.hinstance = initInfo.hInstance;

        VkResult err = vkCreateWin32SurfaceKHR(inst->rend.instance, &createInfo, nullptr, &inst->rend.surface);
        assertExit(err == VK_SUCCESS, "Surface creation failed");

    }

    // Pick physical device
    {

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(inst->rend.instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(inst->rend.instance, &deviceCount, physicalDevices.data());
        assertExit(deviceCount > 0, "No vulkan compatable GPUs");

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

            VkPhysicalDeviceFeatures deviceFeatures{};
            vkGetPhysicalDeviceFeatures(queriedDevice, &deviceFeatures); 
            if (!deviceFeatures.fillModeNonSolid) continue; 

            uint32_t index;
            bool hasGraphicsQueue = vlknh::getQueueFamilyFlagsIndex(queriedDevice, VK_QUEUE_GRAPHICS_BIT, &index);
            bool hasPresentQueue = vlknh::getQueueFamilyPresentIndex(queriedDevice, inst->rend.surface, &index);
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
            if      (hasPresentMode(presentModes, c_vlkn::presentMode))       inst->rend.presentMode = c_vlkn::presentMode;
            else if (hasPresentMode(presentModes, c_vlkn::backupPresentMode)) inst->rend.presentMode = c_vlkn::backupPresentMode;
            else { continue; }

            compatableDevices.push_back(queriedDevice);
        }
        assertExit(compatableDevices.size() > 0, "No application compatable GPUs");
        
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

        vlknh::getQueueFamilyFlagsIndex(inst->rend.physicalDevice, VK_QUEUE_GRAPHICS_BIT, &inst->rend.graphicsQueueIndex);
        vlknh::getQueueFamilyPresentIndex(inst->rend.physicalDevice, inst->rend.surface, &inst->rend.presentQueueIndex);

        VkDeviceQueueCreateInfo queueCreateInfos[2]{};

        float queuePriority = 1.0;
        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].queueFamilyIndex = inst->rend.graphicsQueueIndex;
        queueCreateInfos[0].queueCount = 1;
        queueCreateInfos[0].pQueuePriorities = &queuePriority;

        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].queueFamilyIndex = inst->rend.presentQueueIndex;
        queueCreateInfos[1].queueCount = 1;
        queueCreateInfos[1].pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos;
        createInfo.queueCreateInfoCount = arraySize(queueCreateInfos);
        if (inst->rend.graphicsQueueIndex == inst->rend.presentQueueIndex) createInfo.queueCreateInfoCount -= 1;

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.fillModeNonSolid = VK_TRUE; 
        createInfo.pEnabledFeatures = &deviceFeatures;
        const char* extensionNames[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        createInfo.ppEnabledExtensionNames = extensionNames;
        createInfo.enabledExtensionCount = arraySize(extensionNames);

#ifdef ENABLE_VK_VALIDATION_LAYERS
        createInfo.enabledLayerCount = arraySize(desiredLayers);
        createInfo.ppEnabledLayerNames = desiredLayers;
#else
        createInfo.enabledLayerCount = 0;
#endif

        VkResult err = vkCreateDevice(inst->rend.physicalDevice, &createInfo, nullptr, &inst->rend.device);
        assertExit(err == VK_SUCCESS, "Logical device creation failed");


    }

    // Queue retrieval
    {

        vkGetDeviceQueue(inst->rend.device, inst->rend.graphicsQueueIndex, 0, &inst->rend.graphicsQueue);
        vkGetDeviceQueue(inst->rend.device, inst->rend.presentQueueIndex, 0, &inst->rend.presentQueue);

    }

    Core::createSwapchain(inst, VK_NULL_HANDLE);

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
        assertExit(err == VK_SUCCESS, "Render pass creation failed");

    }

    Core::createFramebuffers(&inst->rend);

    // Command Pool Creation 
    {

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = inst->rend.graphicsQueueIndex;

        VkResult err = vkCreateCommandPool(inst->rend.device, &createInfo, nullptr, &inst->rend.commandPool);
        assertExit(err == VK_SUCCESS, "Command pool creation failed");

    }

    // Command buffer creation
    {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = inst->rend.commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult err = vkAllocateCommandBuffers(inst->rend.device, &allocInfo, &inst->rend.commandBuff);
        assertExit(err == VK_SUCCESS, "Command buffer allocation failed");

    }

    // Descriptor pool creation 
    {

        VkDescriptorPoolSize poolSizeInfo{};
        poolSizeInfo.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizeInfo.descriptorCount = c_vlkn::maxSets; 

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets       = c_vlkn::maxSets; // TODO: prevent the user from creating too many tabs and overflowing the descriptor pool
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSizeInfo;

        VkResult err = vkCreateDescriptorPool(inst->rend.device, &poolInfo, nullptr, &inst->rend.descriptorPool);
        assertExit(err == VK_SUCCESS, "Descriptor pool creation failed");

    }

    // Semaphore and Fence creation 
    {

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult err1 = vkCreateSemaphore(inst->rend.device, &semaphoreInfo, nullptr, &inst->rend.imageReadySemaphore);
        VkResult err2 = vkCreateSemaphore(inst->rend.device, &semaphoreInfo, nullptr, &inst->rend.renderDoneSemaphore);
        assertExit((err1 == VK_SUCCESS || err2 == VK_SUCCESS), "Semaphore creation failed");

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult err = vkCreateFence(inst->rend.device, &fenceInfo, nullptr, &inst->rend.frameFinishedFence);
        assertExit(err == VK_SUCCESS, "Fence Creation failed");

    }

    // Viewports Renderer creation
    {

        // Render Pass Creation
        {

            VkAttachmentDescription colorAttachment{};
            colorAttachment.flags          = 0;
            colorAttachment.format         = c_vlkn::format;
            colorAttachment.samples        = c_vlkn::sampleCount;
            colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format         = c_vlkn::depthFormat; 
            depthAttachment.samples        = c_vlkn::sampleCount;
            depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription colorAttachmentResolve{}; 
            colorAttachmentResolve.format         = c_vlkn::format;
            colorAttachmentResolve.samples        = VK_SAMPLE_COUNT_1_BIT; 
            colorAttachmentResolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; 
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; 
            colorAttachmentResolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; 

            VkAttachmentReference colorAttachmentResolveRef{}; 
            colorAttachmentResolveRef.attachment = 2; 
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount     = 1;
            subpass.pColorAttachments        = &colorAttachmentRef;
            subpass.pDepthStencilAttachment  = &depthAttachmentRef;
            subpass.pResolveAttachments      = &colorAttachmentResolveRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass    = 0;
            dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment, colorAttachmentResolve, };
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.flags           = 0;
            renderPassInfo.attachmentCount = arraySize(attachments);
            renderPassInfo.pAttachments    = attachments;
            renderPassInfo.subpassCount    = 1;
            renderPassInfo.pSubpasses      = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies   = &dependency;

            VkResult err = vkCreateRenderPass(inst->rend.device, &renderPassInfo, nullptr, &inst->vpRend.renderPass);
            assertExit(err == VK_SUCCESS, "Render pass creation failed");

        }

        // Graphics Pipeline Creation
        {

            scopedTimer(t3, inst->gui.stats.perfTimes.getTimer("graphicsPipelineCreation"));

            VkPipelineShaderStageCreateInfo shaderStages[2]{};

            VkShaderModuleCreateInfo vertModuleCreateInfo{};
            vertModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vertModuleCreateInfo.codeSize = sizeof(shader_vert_spv);
            vertModuleCreateInfo.pCode    = (uint32_t*)shader_vert_spv;

            VkShaderModule vertModule;
            VkResult err = vkCreateShaderModule(inst->rend.device, &vertModuleCreateInfo, nullptr, &vertModule);
            assertExit(err == VK_SUCCESS, "Vertex Module Creation failed");

            shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].flags  = 0;
            shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = vertModule;
            shaderStages[0].pName  = "main";

            VkShaderModuleCreateInfo fragModuleCreateInfo{};
            fragModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            fragModuleCreateInfo.codeSize = sizeof(shader_frag_spv);
            fragModuleCreateInfo.pCode = (uint32_t*)shader_frag_spv;

            VkShaderModule fragModule;
            err = vkCreateShaderModule(inst->rend.device, &fragModuleCreateInfo, nullptr, &fragModule);
            assertExit(err == VK_SUCCESS, "Vertex Module Creation failed");

            shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].flags  = 0;
            shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = fragModule;
            shaderStages[1].pName  = "main";

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding   = 0;
            bindingDescription.stride    = sizeof(mload::Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            VkVertexInputAttributeDescription attribDescriptions[2]{};
            attribDescriptions[0].binding  = 0;
            attribDescriptions[0].location = 0;
            attribDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attribDescriptions[0].offset   = offsetof(mload::Vertex, pos);

            attribDescriptions[1].binding  = 0;
            attribDescriptions[1].location = 1;
            attribDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attribDescriptions[1].offset   = offsetof(mload::Vertex, normal);

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = arraySize(attribDescriptions);
            vertexInputInfo.pVertexAttributeDescriptions = attribDescriptions;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount  = 1;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable        = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth               = 1.0f;
            rasterizer.cullMode                = VK_CULL_MODE_FRONT_BIT;
            rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable         = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = c_vlkn::sampleCount;

            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO; 
            depthStencil.depthTestEnable       = VK_TRUE;
            depthStencil.depthWriteEnable      = VK_TRUE;
            depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds        = 0.0f;
            depthStencil.maxDepthBounds        = 1.0f;
            depthStencil.stencilTestEnable     = VK_FALSE;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable    = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable   = VK_FALSE;
            colorBlending.logicOp         = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments    = &colorBlendAttachment;

            VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; 

            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = arraySize(dynamicStates);
            dynamicState.pDynamicStates    = dynamicStates;

            VkPushConstantRange pcRange{};
            pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            pcRange.offset     = 0;
            pcRange.size       = sizeof(PushConstants); 

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.flags                  = 0; 
            pipelineLayoutInfo.setLayoutCount         = 0;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges    = &pcRange;

            err = vkCreatePipelineLayout(inst->rend.device, &pipelineLayoutInfo, nullptr, &inst->vpRend.pipelineLayout);
            assertExit(err == VK_SUCCESS, "Failed to create pipeline layout");

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.flags               = 0;
            pipelineInfo.stageCount          = arraySize(shaderStages);
            pipelineInfo.pStages             = shaderStages;
            pipelineInfo.pVertexInputState   = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pTessellationState  = nullptr;
            pipelineInfo.pViewportState      = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState   = &multisampling;
            pipelineInfo.pDepthStencilState  = &depthStencil;
            pipelineInfo.pColorBlendState    = &colorBlending;
            pipelineInfo.pDynamicState       = &dynamicState;
            pipelineInfo.layout              = inst->vpRend.pipelineLayout;
            pipelineInfo.renderPass          = inst->vpRend.renderPass;
            pipelineInfo.subpass             = 0;
            pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

            err = vkCreateGraphicsPipelines(inst->rend.device, nullptr, 1, &pipelineInfo, nullptr, &inst->vpRend.graphicsPipeline);
            assertExit(err == VK_SUCCESS, "Failed to create graphics pipeline");

            vkDestroyShaderModule(inst->rend.device, vertModule, nullptr);
            vkDestroyShaderModule(inst->rend.device, fragModule, nullptr);

            // Mesh outline pipeline
            vertModuleCreateInfo.codeSize = sizeof(MeshOutlineShader_vert_spv);
            vertModuleCreateInfo.pCode    = (uint32_t*)MeshOutlineShader_vert_spv;

            err = vkCreateShaderModule(inst->rend.device, &vertModuleCreateInfo, nullptr, &vertModule);
            assertExit(err == VK_SUCCESS, "Vertex module vertex creation failed");

            shaderStages[0].module = vertModule;

            fragModuleCreateInfo.codeSize = sizeof(MeshOutlineShader_frag_spv);
            fragModuleCreateInfo.pCode    = (uint32_t*)MeshOutlineShader_frag_spv;

            err = vkCreateShaderModule(inst->rend.device, &fragModuleCreateInfo, nullptr, &fragModule);
            assertExit(err == VK_SUCCESS, "Mesh outline frag module creation failed");

            shaderStages[1].module = fragModule;
            
            vertexInputInfo.vertexAttributeDescriptionCount = 1;
            vertexInputInfo.pVertexAttributeDescriptions    = attribDescriptions;

            rasterizer.polygonMode = VK_POLYGON_MODE_LINE;   

            err = vkCreateGraphicsPipelines(inst->rend.device, nullptr, 1, &pipelineInfo, nullptr, &inst->vpRend.meshOutlinePipeline); 
            assertExit(err == VK_SUCCESS, "Mesh outline graphics pipeline creation failed");

            vkDestroyShaderModule(inst->rend.device, vertModule, nullptr);
            vkDestroyShaderModule(inst->rend.device, fragModule, nullptr);

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

            VkResult err = vkCreateSampler(inst->rend.device, &samplerInfo, nullptr, &inst->vpRend.frameSampler);
            assertExit(err == VK_SUCCESS, "Sampler creation failed");

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

            VkResult err = vkCreateDescriptorSetLayout(inst->rend.device, &layoutInfo, nullptr, &inst->vpRend.descriptorSetLayout);
            assertExit(err == VK_SUCCESS, "Descriptor set layout creation failed");

        }

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
        guiInitInfo.iniPath            = iniPath;

        Gui::init(&guiInitInfo, &inst->gui.styleEx, inst->wind.dpi); 
 
        

        NSVGimage* image = nsvgParseFromFile("Logo.svg", "px", 96);
        assertExit(image != NULL, "Failed to open or read Logo.svg");

        scopedTimer(t2, inst->gui.stats.perfTimes.getTimer("logoRasterize"));

        struct NSVGrasterizer* rast = nsvgCreateRasterizer();
        // Large logo
        {

            inst->gui.logoSize.x = inst->wind.dpi * 300;
            inst->gui.logoSize.y = inst->gui.logoSize.x; 
            int width = (int)inst->gui.logoSize.x;
            int height = width;
            unsigned char* logoImg = (unsigned char*)malloc(width * height * 4);
            nsvgRasterize(rast, image, 0, 0, inst->gui.logoSize.x / 100.0f, logoImg, width, height, width * 4);

            vlknh::TextureImageCreateInfo texImageInfo{};
            texImageInfo.physicalDevice      = inst->rend.physicalDevice;
            texImageInfo.commandPool         = inst->rend.commandPool; 
            texImageInfo.graphicsQueue       = inst->rend.graphicsQueue; 
            texImageInfo.descriptorPool      = inst->rend.descriptorPool;
            texImageInfo.descriptorSetLayout = inst->vpRend.descriptorSetLayout;
            texImageInfo.sampler             = inst->vpRend.frameSampler; 
            texImageInfo.imageSize           = { (uint32_t)width, (uint32_t)height };
            texImageInfo.pPixelData          = logoImg; 

            vlknh::TextureImageResources texImgResources{};
            texImgResources.pTexImg           = &inst->vpRend.logoImg;
            texImgResources.pTexImgMem        = &inst->vpRend.logoImgMem;
            texImgResources.pTexImgView       = &inst->vpRend.logoImgView;

            inst->gui.logoTexID = (ImTextureID)vlknh::createTextureImage(inst->rend.device, texImageInfo, texImgResources); 

            free(logoImg); 

        }

        // Icon
        {

            float& fwidth = inst->gui.styleEx.sizes.titleBarHeight;
            int width = (int)fwidth;
            int height = width;
            unsigned char* logoImg = (unsigned char*)malloc(width * height * 4);
            nsvgRasterize(rast, image, 0, 0, fwidth / 100.0f, logoImg, width, height, width * 4);

            vlknh::TextureImageCreateInfo texImageInfo{};
            texImageInfo.physicalDevice      = inst->rend.physicalDevice;
            texImageInfo.commandPool         = inst->rend.commandPool;
            texImageInfo.graphicsQueue       = inst->rend.graphicsQueue;
            texImageInfo.descriptorPool      = inst->rend.descriptorPool;
            texImageInfo.descriptorSetLayout = inst->vpRend.descriptorSetLayout;
            texImageInfo.sampler             = inst->vpRend.frameSampler;
            texImageInfo.imageSize           = { (uint32_t)width, (uint32_t)height };
            texImageInfo.pPixelData          = logoImg;

            vlknh::TextureImageResources texImgResources{};
            texImgResources.pTexImg     = &inst->vpRend.icoImg;
            texImgResources.pTexImgMem  = &inst->vpRend.icoImgMem;
            texImgResources.pTexImgView = &inst->vpRend.icoImgView;

            inst->gui.icoTexID = (ImTextureID)vlknh::createTextureImage(inst->rend.device, texImageInfo, texImgResources);

            free(logoImg);

        }

        // Mouse Controls Icon
        {

            NSVGimage* mouseControlsImage = nsvgParseFromFile("Mouse-Controls.svg", "px", 96);
            assertExit(mouseControlsImage != NULL, "Failed to open or read Logo.svg");
            inst->gui.mouseControlsSize.x = inst->wind.dpi * 24.0f;
            inst->gui.mouseControlsSize.y = 768.0f / 200.0f * inst->gui.mouseControlsSize.x;
            int width  = (int)inst->gui.mouseControlsSize.x;
            int height = (int)inst->gui.mouseControlsSize.y;
            unsigned char* logoImg = (unsigned char*)malloc(width * height * 4);
            nsvgRasterize(rast, mouseControlsImage, 0, 0, inst->gui.mouseControlsSize.x / 200.0f, logoImg, width, height, width * 4);

            vlknh::TextureImageCreateInfo texImageInfo{};
            texImageInfo.physicalDevice      = inst->rend.physicalDevice;
            texImageInfo.commandPool         = inst->rend.commandPool;
            texImageInfo.graphicsQueue       = inst->rend.graphicsQueue;
            texImageInfo.descriptorPool      = inst->rend.descriptorPool;
            texImageInfo.descriptorSetLayout = inst->vpRend.descriptorSetLayout;
            texImageInfo.sampler             = inst->vpRend.frameSampler;
            texImageInfo.imageSize           = { (uint32_t)width, (uint32_t)height };
            texImageInfo.pPixelData          = logoImg;

            vlknh::TextureImageResources texImgResources{};
            texImgResources.pTexImg     = &inst->vpRend.mouseControlsImg;
            texImgResources.pTexImgMem  = &inst->vpRend.mouseControlsImgMem;
            texImgResources.pTexImgView = &inst->vpRend.mouseControlsImgView;

            inst->gui.mouseControlsTexID = (ImTextureID)vlknh::createTextureImage(inst->rend.device, texImageInfo, texImgResources);

            free(logoImg);
            nsvgDelete(mouseControlsImage);

        }

        nsvgDeleteRasterizer(rast);
        nsvgDelete(image); 

    }

    // Open file if passed to command line
    {

        // If the path is surrounded in quotes
        if (initInfo.fileToOpen[0] == '\"') {
            size_t newStringLen = strlen(initInfo.fileToOpen) - 2; // -2 removes quotes 
            char* fileNoQuotes = (char*)malloc(newStringLen + 1); // +1 for null char 

            if (fileNoQuotes != nullptr) {

                memcpy(fileNoQuotes, &initInfo.fileToOpen[1], newStringLen); 
                fileNoQuotes[newStringLen] = '\0';

                Core::openMeshFile(inst, fileNoQuotes);
                
                free(fileNoQuotes); 

            }
        }
        else {
            Core::openMeshFile(inst, initInfo.fileToOpen);
        }

    }

    // All init code goes above this function. 
    ShowWindow(inst->wind.hwnd, maximized ? SW_MAXIMIZE : initInfo.nCmdShow);
     
}

void App::render(Core::Instance* inst) {
    
    if (IsIconic(inst->wind.hwnd)) { sleepFor(0.005); return; }

    Gui::Commands commands = 0;
    Gui::draw(inst->wind.hwnd, &commands, &inst->gui);

    // Gui::draw will request certain commands and this executes them
    if (commands & Gui::cmd_minimizeWindowBit) {
        ShowWindow(inst->wind.hwnd, SW_MINIMIZE);
        return;
    }
    if (commands & Gui::cmd_maximizeWindowBit) {
        ShowWindow(inst->wind.hwnd, SW_MAXIMIZE);
        return;
    }
    if (commands & Gui::cmd_restoreWindowBit) {
        ShowWindow(inst->wind.hwnd, SW_RESTORE);
        return;
    }
    if (commands & Gui::cmd_closeWindowBit) {
        PostQuitMessage(0);
        return;
    }
    if (commands & Gui::cmd_openDialogBit) {

        // TODO: the following if is wrong, we are using more descriptor sets already than 1. 
        // -1 because we still need one descriptor set for the frame buffer
        if (inst->vpRend.vpInstances.size() >= c_vlkn::maxSets - 1) return;

        char fileName[MAX_PATH]{};

        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = inst->wind.hwnd;
        ofn.lpstrFile = fileName;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(fileName);
        ofn.lpstrFilter = ".obj or .stl\0*.stl;*.obj\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        if (GetOpenFileNameA(&ofn) != TRUE) return;

        Core::openMeshFile(inst, fileName);
        return;

    }

    CORE_ASSERT(inst->vpRend.vpInstances.size() == inst->gui.vpDatas.size());

    // Resize/Close windows if needed
    for (int i = 0; i < inst->gui.vpDatas.size(); ++i) {

        Gui::ViewportGuiData& vpData       = inst->gui.vpDatas[i];
        Core::ViewportInstance& vpInstance = inst->vpRend.vpInstances[i];

        // Close window
        if (!vpData.open) {

            scopedTimer(t1, inst->gui.stats.perfTimes.getTimer("fileClose"));

            vkDeviceWaitIdle(inst->rend.device);

            vkFreeDescriptorSets          (inst->rend.device, inst->rend.descriptorPool, 1, &vpInstance.descriptorSet);
            Core::destroyGeometryData     (inst->rend.device, &vpInstance); 
            Core::destroyVpImageResources (inst->rend.device, &vpInstance); 

            // Remove the window
            inst->vpRend.vpInstances.erase(inst->vpRend.vpInstances.begin() + i);
            inst->gui.vpDatas.erase(inst->gui.vpDatas.begin() + i);
            inst->gui.lastFocusedVp =  inst->gui.vpDatas.size() > 0 ? &inst->gui.vpDatas.back() : nullptr;

            return;

        }
        else if (vpData.resize) {

            vpData.resize = false;

            scopedTimer(t1, inst->gui.stats.perfTimes.getTimer("viewportResize"));

#ifdef DEVINFO
            ++inst->gui.stats.resizeCount;
#endif

            vkDeviceWaitIdle(inst->rend.device);

            // Don't run if this is the first time creating the viewport resources
            if (vpInstance.framebuffer != VK_NULL_HANDLE) Core::destroyVpImageResources(inst->rend.device, &vpInstance); 

            VkExtent2D viewportSize = { (uint32_t)vpData.size.x, (uint32_t)vpData.size.y };
            Core::createVpImageResources(inst, &vpInstance, viewportSize); 

        }

    }

    // Rendering
    {

        scopedTimer(t1, inst->gui.stats.perfTimes.getTimer("renderingCommands"));

        vkWaitForFences(inst->rend.device, 1, &inst->rend.frameFinishedFence, VK_TRUE, UINT64_MAX);
        vkResetFences  (inst->rend.device, 1, &inst->rend.frameFinishedFence);

        uint32_t imageIndex;
        VkResult err = vkAcquireNextImageKHR(inst->rend.device, inst->rend.swapchain, UINT64_MAX, inst->rend.imageReadySemaphore, VK_NULL_HANDLE, &imageIndex);
        CORE_ASSERT(err == VK_SUCCESS && "Failed to get next swapchain image.");

        vkResetCommandBuffer(inst->rend.commandBuff, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        err = vkBeginCommandBuffer(inst->rend.commandBuff, &beginInfo);
        CORE_ASSERT(err == VK_SUCCESS && "Command Buffer begin failed");

        VkRenderPassBeginInfo renderPassInfo{};
        VkClearValue clearValues[2]{};

        // Viewport render pass
        for (int i = 0; i < inst->gui.vpDatas.size(); ++i) {

            Gui::ViewportGuiData& vpData = inst->gui.vpDatas[i];
            if (!vpData.visible) continue; 

            Core::ViewportInstance& vpInstance = inst->vpRend.vpInstances[i]; 

            VkExtent2D viewportExtent = { (uint32_t)vpData.size.x, (uint32_t)vpData.size.y };
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass        = inst->vpRend.renderPass;
            renderPassInfo.framebuffer       = vpInstance.framebuffer;
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = viewportExtent;

            clearValues[0].color = { {18 / 255.0, 18 / 255.0, 18 / 255.0, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

            renderPassInfo.clearValueCount = arraySize(clearValues);
            renderPassInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(inst->rend.commandBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(inst->rend.commandBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, inst->vpRend.graphicsPipeline);

            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = vpData.size.x;
            viewport.height   = vpData.size.y;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(inst->rend.commandBuff, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = viewportExtent;
            vkCmdSetScissor(inst->rend.commandBuff, 0, 1, &scissor);

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(inst->rend.commandBuff, 0, 1, &vpInstance.vertBuff, offsets);
            vkCmdBindIndexBuffer  (inst->rend.commandBuff, vpInstance.indexBuff, 0, VK_INDEX_TYPE_UINT32);

            PushConstants pushConstants; 
            pushConstants.view = vpData.model * glm::translate(glm::mat4(1.0f), -vpData.modelCenter);
            pushConstants.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, vpData.zoomDistance))  * pushConstants.view;
            pushConstants.proj = glm::perspective(glm::radians(45.0f), vpData.size.x / vpData.size.y, 0.01f, vpData.farPlaneClip);


            vkCmdPushConstants(inst->rend.commandBuff, inst->vpRend.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof pushConstants, &pushConstants);

            vkCmdDrawIndexed(inst->rend.commandBuff, vpData.indexCount, 1, 0, 0, 0);
            if (vpData.showEdges) {
                vkCmdBindPipeline(inst->rend.commandBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, inst->vpRend.meshOutlinePipeline); 
                vkCmdDrawIndexed(inst->rend.commandBuff, vpData.indexCount, 1, 0, 0, 0);
            }

            vkCmdEndRenderPass(inst->rend.commandBuff);

        }

        // Main render pass
        {

            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass        = inst->rend.renderPass;
            renderPassInfo.framebuffer       = inst->rend.framebuffers[imageIndex];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = inst->rend.windowImageExtent;

            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = clearValues;


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

        }
         
    }

    float timeError = inst->wind.refreshInterval - ImGui::GetIO().DeltaTime;

    inst->rend.frameWaitTime += timeError;
    if (inst->rend.frameWaitTime < 0.0f ) inst->rend.frameWaitTime = 0.0f; 
    sleepFor(inst->rend.frameWaitTime); // This is needed because I want to have low input lag without rendering unnecessary frames. Serisouly, comment out this line and change present mode to VK_PRESENT_MODE_FIFO_KHR and see the difference. 

}

void App::close(Core::Instance* inst) {

    // IMPORTANT: All vulkan clean up must happen after this line.
    vkDeviceWaitIdle(inst->rend.device);
    // IMPORTANT: All vulkan clean up must happen after this line. Leave extra space below.


     // Viewports cleanup
    {

        vkDestroyDescriptorSetLayout (inst->rend.device, inst->vpRend.descriptorSetLayout,    nullptr);
        vkDestroySampler             (inst->rend.device, inst->vpRend.frameSampler,           nullptr);
        vkDestroyPipeline            (inst->rend.device, inst->vpRend.graphicsPipeline,       nullptr);
        vkDestroyPipeline            (inst->rend.device, inst->vpRend.meshOutlinePipeline,    nullptr);
        vkDestroyPipelineLayout      (inst->rend.device, inst->vpRend.pipelineLayout,         nullptr);
        vkDestroyRenderPass          (inst->rend.device, inst->vpRend.renderPass,             nullptr);

        for (Core::ViewportInstance& vpInstance : inst->vpRend.vpInstances) {

            Core::destroyGeometryData     (inst->rend.device, &vpInstance); 
            Core::destroyVpImageResources (inst->rend.device, &vpInstance); 

        }

    }

    const char* iniPath = ImGui::GetIO().IniFilename; 
    Gui::destroy();

    // set custom out data. 
    {

        CustomIniData dataOut; 

        dataOut.sensitivity     = (int)inst->gui.sensitivity;
        dataOut.windowMaximized = !!IsZoomed(inst->wind.hwnd);

        WINDOWPLACEMENT wp;
        GetWindowPlacement(inst->wind.hwnd, &wp);

        dataOut.windowWidth     = wp.rcNormalPosition.right  - wp.rcNormalPosition.left;
        dataOut.windowHeight    = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        dataOut.windowPosX      = wp.rcNormalPosition.left; 
        if (dataOut.windowPosX < 0) dataOut.windowPosX = 0;
        dataOut.windowPosY      = wp.rcNormalPosition.top;
        if (dataOut.windowPosY < 0) dataOut.windowPosY = 0;

        setCustomIniData(dataOut, iniPath); 

    }

    free((void*)iniPath); 

    vkDestroyImageView      (inst->rend.device, inst->vpRend.icoImgView,           nullptr);
    vkDestroyImage          (inst->rend.device, inst->vpRend.icoImg,               nullptr);
    vkFreeMemory            (inst->rend.device, inst->vpRend.icoImgMem,            nullptr);
    vkDestroyImageView      (inst->rend.device, inst->vpRend.logoImgView,          nullptr);
    vkDestroyImage          (inst->rend.device, inst->vpRend.logoImg,              nullptr);
    vkFreeMemory            (inst->rend.device, inst->vpRend.logoImgMem,           nullptr);
    vkDestroyImageView      (inst->rend.device, inst->vpRend.mouseControlsImgView, nullptr);
    vkDestroyImage          (inst->rend.device, inst->vpRend.mouseControlsImg,     nullptr);
    vkFreeMemory            (inst->rend.device, inst->vpRend.mouseControlsImgMem,  nullptr);
    vkDestroyFence          (inst->rend.device, inst->rend.frameFinishedFence,     nullptr);
    vkDestroySemaphore      (inst->rend.device, inst->rend.renderDoneSemaphore,    nullptr);
    vkDestroySemaphore      (inst->rend.device, inst->rend.imageReadySemaphore,    nullptr);
    vkDestroyDescriptorPool (inst->rend.device, inst->rend.descriptorPool,         nullptr);
    vkDestroyCommandPool    (inst->rend.device, inst->rend.commandPool,            nullptr);
    vkDestroyRenderPass     (inst->rend.device, inst->rend.renderPass,             nullptr);
    
    Core::cleanupSwapchainResources(&inst->rend); 
    vkDestroySwapchainKHR(inst->rend.device, inst->rend.swapchain, nullptr);

    vkDestroyDevice(inst->rend.device, nullptr);
    vkDestroySurfaceKHR(inst->rend.instance, inst->rend.surface, nullptr);
#ifdef ENABLE_VK_VALIDATION_LAYERS
    vlknh::destroyDebugUtilsMessengerEXT(inst->rend.instance, inst->rend.debugMessenger, nullptr);
#endif 
    vkDestroyInstance(inst->rend.instance, nullptr);

}