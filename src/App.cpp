#include "App.hpp"
#include "CoreConstants.hpp"

#include <ShellScalingApi.h>

#include <VulkanHelpers.hpp>
#include <Timer.hpp>
#include <ModelLoader.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

// TODO move the following struct and static
struct PushConstants {
    glm::mat4 model, view, proj; 
};

void App::init(Core::Instance* inst, InstanceInfo* initInfo) {

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

        int startPosX = int(0.5 * (mntrWidth  - inst->wind.size.x));
        int startPosY = int(0.5 * (mntrHeight - inst->wind.size.y));

        inst->wind.hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            L"Vulkan App",
            WS_OVERLAPPEDWINDOW,
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

        SetWindowLongPtr(inst->wind.hwnd, GWLP_USERDATA, (LONG_PTR)inst);

        DEVMODE devMode;
        devMode.dmSize = sizeof(devMode);
        BOOL err = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
        CORE_ASSERT(err && "EnumDisplaySettings failed");
        inst->wind.refreshInterval = 1.0f / devMode.dmDisplayFrequency;

    }

    // Vulkan Instance creation 
    {

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "SimpleViewer3D";
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
            CORE_ASSERT(!"An extension in entensionNames does not exist.");
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
        CORE_ASSERT(err == VK_SUCCESS && "Vulkan instance creation failed");

#ifdef ENABLE_VK_VALIDATION_LAYERS

        err = Core::CreateDebugUtilsMessengerEXT(inst->rend.instance, &validationCreateInfo, nullptr, &inst->rend.debugMessenger);
        CORE_ASSERT(err == VK_SUCCESS && "CreateDebugUtilsMessengerEXT failed");

#endif 
    }

    // Vulkan surface creation
    {

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = inst->wind.hwnd;
        createInfo.hinstance = initInfo->hInstance;

        VkResult err = vkCreateWin32SurfaceKHR(inst->rend.instance, &createInfo, nullptr, &inst->rend.surface);
        CORE_ASSERT(err == VK_SUCCESS && "Surface creation failed");

    }

    // Pick physical device
    {

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(inst->rend.instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(inst->rend.instance, &deviceCount, physicalDevices.data());
        CORE_ASSERT(deviceCount > 0);

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
                vkGetPhysicalDeviceSurfaceSupportKHR(queriedDevice, i, inst->rend.surface, &presentSupport);
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
            vkGetPhysicalDeviceSurfaceFormatsKHR(queriedDevice, inst->rend.surface, &formatCount, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(queriedDevice, inst->rend.surface, &formatCount, formats.data());
            for (const VkSurfaceFormatKHR& format : formats) {
                if (format.format == c_vlkn::format && format.colorSpace == c_vlkn::colorSpace) {
                    formatsAdequate = true;
                    break;
                }
            }

            uint32_t presentModeCount;
            bool presentModeAdequate = false;
            vkGetPhysicalDeviceSurfacePresentModesKHR(queriedDevice, inst->rend.surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(queriedDevice, inst->rend.surface, &presentModeCount, presentModes.data());
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
        CORE_ASSERT(compatableDevices.size() > 0);
        for (VkPhysicalDevice compatableDevice : compatableDevices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(compatableDevice, &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                inst->rend.physicalDevice = compatableDevice;
                goto discreteFound;
            }
        }
        inst->rend.physicalDevice = compatableDevices[0];
    discreteFound:;

    }

    // Logical device creation
    {

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
        CORE_ASSERT(err == VK_SUCCESS && "Logical device creation failed");


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
        colorAttachment.flags = 0;
        colorAttachment.format = c_vlkn::format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

        VkResult err = vkCreateRenderPass(inst->rend.device, &renderPassInfo, nullptr, &inst->rend.renderPass);
        CORE_ASSERT(err == VK_SUCCESS && "Render pass creation failed");

    }

    Core::createFramebuffers(&inst->rend);

    // Command Pool Creation 
    {

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = inst->rend.graphicsQueueIndex;

        VkResult err = vkCreateCommandPool(inst->rend.device, &createInfo, nullptr, &inst->rend.commandPool);
        CORE_ASSERT(err == VK_SUCCESS && "Command pool creation failed");

    }

    // Command buffer creation
    {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = inst->rend.commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult err = vkAllocateCommandBuffers(inst->rend.device, &allocInfo, &inst->rend.commandBuff);
        CORE_ASSERT(err == VK_SUCCESS && "Command buffer allocation failed");

    }

    // Decriptor pool creation 
    {

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets       = 2;
        poolInfo.poolSizeCount = 0;
        poolInfo.pPoolSizes    = nullptr;

        VkResult err = vkCreateDescriptorPool(inst->rend.device, &poolInfo, nullptr, &inst->rend.descriptorPool);
        CORE_ASSERT(err == VK_SUCCESS && "Descriptor pool creation failed");

    }

    // Semaphore and Fence creation 
    {

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult err1 = vkCreateSemaphore(inst->rend.device, &semaphoreInfo, nullptr, &inst->rend.imageReadySemaphore);
        VkResult err2 = vkCreateSemaphore(inst->rend.device, &semaphoreInfo, nullptr, &inst->rend.renderDoneSemaphore);
        CORE_ASSERT((err1 == VK_SUCCESS || err2 == VK_SUCCESS) && "Semaphore creation failed");

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult err = vkCreateFence(inst->rend.device, &fenceInfo, nullptr, &inst->rend.frameFinishedFence);
        CORE_ASSERT(err == VK_SUCCESS && "Fence Creation failed");

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

    }

    // viewport (NOT imgui viewport as in a separate window). This is where the 3D model is drawn.
    {

        // Render Pass Creation
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
            colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format         = c_vlkn::depthFormat; 
            depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount     = 1;
            subpass.pColorAttachments        = &colorAttachmentRef;
            subpass.pDepthStencilAttachment  = &depthAttachmentRef;
            subpass.pResolveAttachments      = nullptr;

            VkSubpassDependency dependency{};
            dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass    = 0;
            dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
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
            CORE_ASSERT(err == VK_SUCCESS && "Render pass creation failed");

        }

        // Graphics Pipeline Creation
        {

            VkPipelineShaderStageCreateInfo shaderStages[2]{};

            VkShaderModule vertModule = vlknh::createShaderModule(inst->rend.device, "res/shaders/shader.vert.spv");
            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].flags = 0;
            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = vertModule;
            shaderStages[0].pName = "main";

            VkShaderModule fragModule = vlknh::createShaderModule(inst->rend.device, "res/shaders/shader.frag.spv");
            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].flags = 0;
            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = fragModule;
            shaderStages[1].pName = "main";

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(mload::Vertex);
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

            VkResult err = vkCreatePipelineLayout(inst->rend.device, &pipelineLayoutInfo, nullptr, &inst->vpRend.pipelineLayout);
            CORE_ASSERT(err == VK_SUCCESS && "Failed to create pipeline layout");

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
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            err = vkCreateGraphicsPipelines(inst->rend.device, nullptr, 1, &pipelineInfo, nullptr, &inst->vpRend.graphicsPipeline);
            CORE_ASSERT(err == VK_SUCCESS && "Failed to create graphics pipeline");

            vkDestroyShaderModule(inst->rend.device, vertModule, nullptr);
            vkDestroyShaderModule(inst->rend.device, fragModule, nullptr);

        }

        // TODO: write vertexdata cube in c array. 
        // BOOKMARK: fill this shit out.

        mload::Vertex cubeVertices[] = {
            
            // right
            { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f } },

            // top
            { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f } },
                                            
            // front
            { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f } },

            // left
            { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f } },
            { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f } },
            { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f } },
            { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f } },

            // bottom
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f } },

            // back
            { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f } },

        };
        uint32_t cubeIndices[] = {
            
            // right
            0, 1, 2, 2, 3, 0,

            // top
            4, 5, 6, 6, 7, 4,

            // front
            8, 9, 10, 10, 11, 8,

            // left
            12, 13, 14, 14, 15, 12,

            // bottom
            16, 17, 18, 18, 19, 16,

            // back
            20, 21, 22, 22, 23, 20,

        };

        Core::VertexIndexBuffersInfo buffsInfo{};
        buffsInfo.vertexData     = cubeVertices;
        buffsInfo.vertexDataSize = sizeof cubeVertices;
        buffsInfo.indexData      = cubeIndices;
        buffsInfo.indexDataSize  = sizeof cubeIndices;

        Core::createVertexIndexBuffers(inst, &buffsInfo);

        inst->vpRend.triangleCount = arraySize(cubeIndices);

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
            CORE_ASSERT(err == VK_SUCCESS && "Sampler creation failed");

        }

        // Texture descriptor set creation
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

            vkCreateDescriptorSetLayout(inst->rend.device, &layoutInfo, nullptr, &inst->vpRend.descriptorSetLayout); 

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool     = inst->rend.descriptorPool; 
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts        = &inst->vpRend.descriptorSetLayout;

            vkAllocateDescriptorSets(inst->rend.device, &allocInfo, &inst->vpRend.descriptorSet);

        }

    }

    // All init code goes above this function. 
    ShowWindow(inst->wind.hwnd, initInfo->nCmdShow);
     
}

void App::render(Core::Instance* inst) {
    
    if (IsIconic(inst->wind.hwnd)) { sleepFor(0.005); return; }

    Gui::Commands cmdList[Gui::cmdCount];
    Gui::draw(inst->wind.hwnd, cmdList, &inst->gui);
    
    VkExtent2D viewportExtent = { (uint32_t)inst->gui.vpData.viewportSize.x, (uint32_t)inst->gui.vpData.viewportSize.y };

    // Gui::draw will request certain commands and this executes them
    for (Gui::Commands* cmd = cmdList; *cmd != Gui::cmd_null; ++cmd) {
        switch (*cmd) {

        case Gui::cmd_minimizeWindow: {
            ShowWindow(inst->wind.hwnd, SW_MINIMIZE);
            return;
        }
        case Gui::cmd_maximizeWindow: {
            ShowWindow(inst->wind.hwnd, SW_MAXIMIZE);
            return;
        }
        case Gui::cmd_restoreWindow: {
            ShowWindow(inst->wind.hwnd, SW_RESTORE);
            return;
        }
        case Gui::cmd_closeWindow: {
            PostQuitMessage(0);
            return;
        }
        case Gui::cmd_openDialog: {

            char fileName[MAX_PATH]{};

            OPENFILENAMEA ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = inst->wind.hwnd;
            ofn.lpstrFile = fileName;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(fileName);
            ofn.lpstrFilter = "STL files\0*.stl\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

            if (GetOpenFileNameA(&ofn) != TRUE) return;

            vkQueueWaitIdle(inst->rend.graphicsQueue);

            vkFreeMemory(inst->rend.device, inst->vpRend.vertBuffMem, nullptr);
            vkDestroyBuffer(inst->rend.device, inst->vpRend.vertBuff, nullptr);
            vkFreeMemory(inst->rend.device, inst->vpRend.indexBuffMem, nullptr);
            vkDestroyBuffer(inst->rend.device, inst->vpRend.indexBuff, nullptr);

            std::vector<mload::Vertex> vertices;
            std::vector<uint32_t>      indices;

            mload::openModel(fileName, &vertices, &indices);

            Core::VertexIndexBuffersInfo buffsInfo{};
            buffsInfo.vertexData = vertices.data();
            buffsInfo.vertexDataSize = vertices.size() * sizeof mload::Vertex;
            buffsInfo.indexData = indices.data();
            buffsInfo.indexDataSize = indices.size() * sizeof uint32_t;

            Core::createVertexIndexBuffers(inst, &buffsInfo);

            inst->vpRend.triangleCount = (uint32_t)indices.size(); 

            break;
        }
        case Gui::cmd_resizeViewport: {

            ++inst->gui.stats.resizeCount;

            vkDeviceWaitIdle(inst->rend.device);

            // Don't run if this is the first time creating the viewport resources
            if (inst->vpRend.framebuffer != VK_NULL_HANDLE) {

                vkDestroyFramebuffer(inst->rend.device, inst->vpRend.framebuffer, nullptr);
                vkDestroyImageView(inst->rend.device, inst->vpRend.depthImageView, nullptr);
                vkFreeMemory(inst->rend.device, inst->vpRend.depthImageMem, nullptr);
                vkDestroyImage(inst->rend.device, inst->vpRend.depthImage, nullptr);
                vkDestroyImageView(inst->rend.device, inst->vpRend.imageView, nullptr);
                vkFreeMemory(inst->rend.device, inst->vpRend.imageMem, nullptr);
                vkDestroyImage(inst->rend.device, inst->vpRend.image, nullptr);

            }

            // Image creation
            {

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.flags = 0;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.format = c_vlkn::format;
                imageInfo.extent = { viewportExtent.width, viewportExtent.height, 1 };
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                imageInfo.queueFamilyIndexCount = 1;
                imageInfo.pQueueFamilyIndices = &inst->rend.graphicsQueueIndex;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                VkResult err = vkCreateImage(inst->rend.device, &imageInfo, nullptr, &inst->vpRend.image);
                CORE_ASSERT(err == VK_SUCCESS && "Image creation failed");

            }

            // Image memory creation
            {

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(inst->rend.device, inst->vpRend.image, &memRequirements);
                VkPhysicalDeviceMemoryProperties memProps;
                vkGetPhysicalDeviceMemoryProperties(inst->rend.physicalDevice, &memProps);

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
                VkResult err = vkAllocateMemory(inst->rend.device, &allocInfo, nullptr, &inst->vpRend.imageMem);
                CORE_ASSERT(err == VK_SUCCESS && "Buffer allocation failed");

                vkBindImageMemory(inst->rend.device, inst->vpRend.image, inst->vpRend.imageMem, 0);

            }

            // Image view creation
            {

                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.flags = 0;
                createInfo.image = inst->vpRend.image;
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

                VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &inst->vpRend.imageView);
                CORE_ASSERT(err == VK_SUCCESS && "Image view creation failed");

            }

            // Depth image creation
            {

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.flags = 0;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.format = c_vlkn::depthFormat;
                imageInfo.extent = { viewportExtent.width, viewportExtent.height, 1 };
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                imageInfo.queueFamilyIndexCount = 1;
                imageInfo.pQueueFamilyIndices = &inst->rend.graphicsQueueIndex;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                VkResult err = vkCreateImage(inst->rend.device, &imageInfo, nullptr, &inst->vpRend.depthImage);
                CORE_ASSERT(err == VK_SUCCESS && "Depth image creation failed");

            }

            // Image memory creation
            {

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(inst->rend.device, inst->vpRend.depthImage, &memRequirements);
                VkPhysicalDeviceMemoryProperties memProps;
                vkGetPhysicalDeviceMemoryProperties(inst->rend.physicalDevice, &memProps);

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
                VkResult err = vkAllocateMemory(inst->rend.device, &allocInfo, nullptr, &inst->vpRend.depthImageMem);
                CORE_ASSERT(err == VK_SUCCESS && "Buffer allocation failed");

                vkBindImageMemory(inst->rend.device, inst->vpRend.depthImage, inst->vpRend.depthImageMem, 0);

            }

            // Depth image view creation
            {

                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.flags = 0;
                createInfo.image = inst->vpRend.depthImage;
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = c_vlkn::depthFormat;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &inst->vpRend.depthImageView);
                CORE_ASSERT(err == VK_SUCCESS && "Depth image view creation failed");

            }

            // Update Descriptor Set
            {

                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = inst->vpRend.frameSampler;
                imageInfo.imageView = inst->vpRend.imageView;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkWriteDescriptorSet writeDescriptor{};
                writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptor.dstSet = inst->vpRend.descriptorSet;
                writeDescriptor.dstBinding = 0;
                writeDescriptor.dstArrayElement = 0;
                writeDescriptor.descriptorCount = 1;
                writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptor.pImageInfo = &imageInfo;

                vkUpdateDescriptorSets(inst->rend.device, 1, &writeDescriptor, 0, nullptr);

                inst->gui.vpData.framebufferTexID = (ImTextureID)inst->vpRend.descriptorSet;

            }

            // Framebuffer creation
            {

                VkImageView attachments[] = { inst->vpRend.imageView, inst->vpRend.depthImageView };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = inst->vpRend.renderPass;
                framebufferInfo.attachmentCount = arraySize(attachments);
                framebufferInfo.pAttachments = attachments;
                framebufferInfo.width = viewportExtent.width;
                framebufferInfo.height = viewportExtent.height;
                framebufferInfo.layers = 1;

                VkResult err = vkCreateFramebuffer(inst->rend.device, &framebufferInfo, nullptr, &inst->vpRend.framebuffer);
                CORE_ASSERT(err == VK_SUCCESS && "Frambuffer creation failed");

            }

            break;
        }

        }
    }
    {} // Fixes visual studio bug with collapsing scopes

    // Rendering
    {

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
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = inst->vpRend.renderPass;
        renderPassInfo.framebuffer       = inst->vpRend.framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = viewportExtent;

        VkClearValue clearValues[2]{};
        clearValues[0].color = {{18 / 255.0, 18 / 255.0, 18 / 255.0, 1.0f}};
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = arraySize(clearValues);
        renderPassInfo.pClearValues    = clearValues;

        // Viewport render pass
        {

            vkCmdBeginRenderPass(inst->rend.commandBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(inst->rend.commandBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, inst->vpRend.graphicsPipeline);

            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = inst->gui.vpData.viewportSize.x;
            viewport.height   = inst->gui.vpData.viewportSize.y;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(inst->rend.commandBuff, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = viewportExtent;
            vkCmdSetScissor(inst->rend.commandBuff, 0, 1, &scissor);

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(inst->rend.commandBuff, 0, 1, &inst->vpRend.vertBuff, offsets);
            vkCmdBindIndexBuffer  (inst->rend.commandBuff, inst->vpRend.indexBuff, 0, VK_INDEX_TYPE_UINT32);

            PushConstants pushConstants;
            pushConstants.model = glm::eulerAngleXY(inst->gui.vpData.orbitAngle.x, inst->gui.vpData.orbitAngle.y);
            pushConstants.view  = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
            pushConstants.proj  = glm::perspective(glm::radians(45.0f), inst->gui.vpData.viewportSize.x / inst->gui.vpData.viewportSize.y, 0.1f, 50.0f);
            vkCmdPushConstants(inst->rend.commandBuff, inst->vpRend.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

            vkCmdDrawIndexed(inst->rend.commandBuff, inst->vpRend.triangleCount, 1, 0, 0, 0);

            vkCmdEndRenderPass(inst->rend.commandBuff);

        }

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = inst->rend.renderPass;
        renderPassInfo.framebuffer       = inst->rend.framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = inst->rend.windowImageExtent;

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = clearValues;
        
        // Main render pass
        {

            vkCmdBeginRenderPass(inst->rend.commandBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inst->rend.commandBuff);

            vkCmdEndRenderPass(inst->rend.commandBuff);

            err = vkEndCommandBuffer(inst->rend.commandBuff);
            CORE_ASSERT(err == VK_SUCCESS && "Command buffer end failed");

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &inst->rend.imageReadySemaphore;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &inst->rend.commandBuff;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &inst->rend.renderDoneSemaphore;

            err = vkQueueSubmit(inst->rend.graphicsQueue, 1, &submitInfo, inst->rend.frameFinishedFence);
            CORE_ASSERT(err == VK_SUCCESS && "Queue submit failed");

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &inst->rend.renderDoneSemaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &inst->rend.swapchain;
            presentInfo.pImageIndices = &imageIndex;

            err = vkQueuePresentKHR(inst->rend.presentQueue, &presentInfo);
            CORE_ASSERT(err == VK_SUCCESS && "Presenting failed");

        }

        float* timesArray = inst->gui.stats.frameWaitTimesGraph; 
        constexpr uint32_t sampleCount = arraySize(inst->gui.stats.frameWaitTimesGraph);
        for (float* value = timesArray; value < timesArray + sampleCount; ++value)
            value[0] = value[1];

        double timeError = inst->wind.refreshInterval - ImGui::GetIO().DeltaTime;
        timesArray[sampleCount - 1] = (float)timeError;
        inst->vpRend.frameWaitTime += 0.5 * timeError;

        sleepFor(inst->vpRend.frameWaitTime);

    }

}

void App::close(Core::Instance* inst) {

    // IMPORTANT: All vulkan clean up must happen after this line.
    vkDeviceWaitIdle(inst->rend.device);
    // IMPORTANT: All vulkan clean up must happen after this line. Leave extra space below.


     // Viewport cleanup
    {

        vkDestroyDescriptorSetLayout(inst->rend.device, inst->vpRend.descriptorSetLayout, nullptr);
        vkDestroySampler            (inst->rend.device, inst->vpRend.frameSampler,        nullptr);
        vkFreeMemory                (inst->rend.device, inst->vpRend.vertBuffMem,         nullptr);
        vkDestroyBuffer             (inst->rend.device, inst->vpRend.vertBuff,            nullptr);
        vkFreeMemory                (inst->rend.device, inst->vpRend.indexBuffMem,        nullptr);
        vkDestroyBuffer             (inst->rend.device, inst->vpRend.indexBuff,           nullptr);
        vkDestroyPipeline           (inst->rend.device, inst->vpRend.graphicsPipeline,    nullptr);
        vkDestroyPipelineLayout     (inst->rend.device, inst->vpRend.pipelineLayout,      nullptr);
        vkDestroyFramebuffer        (inst->rend.device, inst->vpRend.framebuffer,         nullptr);
        vkDestroyRenderPass         (inst->rend.device, inst->vpRend.renderPass,          nullptr);
        vkDestroyImageView          (inst->rend.device, inst->vpRend.depthImageView,      nullptr);
        vkFreeMemory                (inst->rend.device, inst->vpRend.depthImageMem,       nullptr);
        vkDestroyImage              (inst->rend.device, inst->vpRend.depthImage,          nullptr);
        vkDestroyImageView          (inst->rend.device, inst->vpRend.imageView,           nullptr);
        vkFreeMemory                (inst->rend.device, inst->vpRend.imageMem,            nullptr);
        vkDestroyImage              (inst->rend.device, inst->vpRend.image,               nullptr);

    }

    Gui::destroy(); 

    vkDestroyFence         (inst->rend.device, inst->rend.frameFinishedFence,  nullptr);
    vkDestroySemaphore     (inst->rend.device, inst->rend.renderDoneSemaphore, nullptr);
    vkDestroySemaphore     (inst->rend.device, inst->rend.imageReadySemaphore, nullptr);
    vkDestroyDescriptorPool(inst->rend.device, inst->rend.descriptorPool,      nullptr);
    vkDestroyCommandPool   (inst->rend.device, inst->rend.commandPool,         nullptr);

    vkDestroyRenderPass(inst->rend.device, inst->rend.renderPass, nullptr);
    
    Core::cleanupSwapchainResources(&inst->rend); 
    vkDestroySwapchainKHR(inst->rend.device, inst->rend.swapchain, nullptr);

    vkDestroyDevice(inst->rend.device, nullptr);
    vkDestroySurfaceKHR(inst->rend.instance, inst->rend.surface, nullptr);
#ifdef ENABLE_VK_VALIDATION_LAYERS
    Core::DestroyDebugUtilsMessengerEXT(inst->rend.instance, inst->rend.debugMessenger, nullptr);
#endif 
    vkDestroyInstance(inst->rend.instance, nullptr);

}