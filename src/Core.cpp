#include "App.hpp"
#include "CoreConstants.hpp"

#include <ModelLoader.hpp>

#include <VulkanHelpers.hpp>

#include <algorithm>

void Core::createSwapchain(Instance* inst, VkSwapchainKHR oldSwapchain) {

    // Swapchain creation 
    {

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.flags = 0;
        createInfo.surface = inst->rend.surface;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(inst->rend.physicalDevice, inst->rend.surface, &capabilities);

        CORE_ASSERT(c_vlkn::imageCount >= capabilities.minImageCount);

        createInfo.minImageCount   = c_vlkn::imageCount;
        createInfo.imageFormat     = c_vlkn::format;
        createInfo.imageColorSpace = c_vlkn::colorSpace;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            createInfo.imageExtent = capabilities.currentExtent;
        }
        else {
            createInfo.imageExtent = {
                std::clamp((unsigned)inst->wind.m_size.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp((unsigned)inst->wind.m_size.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };

        }

        inst->rend.windowImageExtent = createInfo.imageExtent;

        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { inst->rend.graphicsQueueIndex, inst->rend.presentQueueIndex };
        if (inst->rend.graphicsQueueIndex == inst->rend.presentQueueIndex) {
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
        createInfo.presentMode = inst->rend.presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult err = vkCreateSwapchainKHR(inst->rend.device, &createInfo, nullptr, &inst->rend.swapchain);
        CORE_ASSERT(err == VK_SUCCESS && "Swapchain creation failed");

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
            createInfo.flags = 0;
            createInfo.image = inst->rend.swapchainImages[i];
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

            VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &inst->rend.swapchainImageViews[i]);
            CORE_ASSERT(err == VK_SUCCESS && "Image view creation failed");

        }

    }

}
void Core::createFramebuffers(VlknRenderInstance* rend) {

    rend->framebuffers = new VkFramebuffer[rend->imageCount];

    for (uint32_t i = 0; i < rend->imageCount; ++i) {

        VkImageView attachments[] = { rend->swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = rend->renderPass;
        framebufferInfo.attachmentCount = arraySize(attachments);
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = rend->windowImageExtent.width;
        framebufferInfo.height          = rend->windowImageExtent.height;
        framebufferInfo.layers          = 1;

        VkResult err = vkCreateFramebuffer(rend->device, &framebufferInfo, nullptr, &rend->framebuffers[i]);
        CORE_ASSERT(err == VK_SUCCESS && "Frambuffer creation failed");

    }

}
void Core::cleanupSwapchainResources(VlknRenderInstance* rend) {

    for (unsigned i = 0; i < rend->imageCount; ++i) {
        vkDestroyFramebuffer(rend->device, rend->framebuffers[i], nullptr);
    }
    delete[] rend->framebuffers;
    for (unsigned i = 0; i < rend->imageCount; ++i) {
        vkDestroyImageView(rend->device, rend->swapchainImageViews[i], nullptr);
    }
    delete[] rend->swapchainImageViews;
    delete[] rend->swapchainImages;

}
void Core::recreateSwapchain(Instance* inst) {

    vkDeviceWaitIdle(inst->rend.device);

    cleanupSwapchainResources(&inst->rend);
    vkDestroySwapchainKHR(inst->rend.device, inst->rend.swapchain, nullptr);

    createSwapchain(inst);
    createFramebuffers(&inst->rend);

}

void Core::createGeometryData(Instance* inst, ViewportInstance* vpInst, VertexIndexBuffersInfo* buffsInfo) {

    VkBuffer       vertStageBuff, indexStageBuff;
    VkDeviceMemory vertStageMem,  indexStageMem;

    // Vertex buffer
    {

        vlknh::BufferCreateInfo buffInfo{};
        buffInfo.physicalDevice = inst->rend.physicalDevice;
        buffInfo.size           = buffsInfo->vertexDataSize;
        buffInfo.usage          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffInfo.properties     = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vlknh::createBuffer(inst->rend.device, buffInfo, &vertStageBuff, &vertStageMem);
        vlknh::loadBuffer(inst->rend.device, vertStageMem, buffsInfo->vertexData, buffsInfo->vertexDataSize);

        buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vlknh::createBuffer(inst->rend.device, buffInfo, &vpInst->vertBuff, &vpInst->vertBuffMem);

    }

    // Indexbuffer
    {

        vlknh::BufferCreateInfo buffInfo{};
        buffInfo.physicalDevice = inst->rend.physicalDevice;
        buffInfo.size           = buffsInfo->indexDataSize;
        buffInfo.usage          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffInfo.properties     = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vlknh::createBuffer(inst->rend.device, buffInfo, &indexStageBuff, &indexStageMem);
        vlknh::loadBuffer(inst->rend.device, indexStageMem, buffsInfo->indexData, buffsInfo->indexDataSize);

        buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vlknh::createBuffer(inst->rend.device, buffInfo, &vpInst->indexBuff, &vpInst->indexBuffMem);

    }

    VkCommandBuffer singleTimeBuff; 
    vlknh::SingleTimeCommandBuffer::begin  (inst->rend.device, inst->rend.commandPool, &singleTimeBuff);
    vlknh::SingleTimeCommandBuffer::copy   (singleTimeBuff, buffsInfo->vertexDataSize, vertStageBuff, vpInst->vertBuff);
    vlknh::SingleTimeCommandBuffer::copy   (singleTimeBuff, buffsInfo->indexDataSize, indexStageBuff, vpInst->indexBuff);
    vlknh::SingleTimeCommandBuffer::submit (inst->rend.device, singleTimeBuff, inst->rend.commandPool, inst->rend.graphicsQueue);

    vkQueueWaitIdle(inst->rend.graphicsQueue);

    vkFreeCommandBuffers(inst->rend.device, inst->rend.commandPool, 1, &singleTimeBuff);
    vkFreeMemory        (inst->rend.device, vertStageMem,   nullptr);
    vkDestroyBuffer     (inst->rend.device, vertStageBuff,  nullptr);
    vkFreeMemory        (inst->rend.device, indexStageMem,  nullptr);
    vkDestroyBuffer     (inst->rend.device, indexStageBuff, nullptr);

}
void Core::createVpImageResources(Instance *inst, ViewportInstance* vpInst, const VkExtent2D size) {

    // Image creation
    {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = 0;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = c_vlkn::format;
        imageInfo.extent = { size.width, size.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = 1;
        imageInfo.pQueueFamilyIndices = &inst->rend.graphicsQueueIndex;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult err = vkCreateImage(inst->rend.device, &imageInfo, nullptr, &vpInst->image);
        CORE_ASSERT(err == VK_SUCCESS && "Image creation failed");

    }

    // Image memory creation
    {

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(inst->rend.device, vpInst->image, &memRequirements);
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
        VkResult err = vkAllocateMemory(inst->rend.device, &allocInfo, nullptr, &vpInst->imageMem);
        CORE_ASSERT(err == VK_SUCCESS && "Buffer allocation failed");

        vkBindImageMemory(inst->rend.device, vpInst->image, vpInst->imageMem, 0);

    }

    // Image view creation
    {

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.image = vpInst->image;
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

        VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &vpInst->imageView);
        CORE_ASSERT(err == VK_SUCCESS && "Image view creation failed");

    }

    // Color image creation
    {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = 0;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = c_vlkn::format;
        imageInfo.extent = { size.width, size.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = c_vlkn::sampleCount;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = 1;
        imageInfo.pQueueFamilyIndices = &inst->rend.graphicsQueueIndex;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult err = vkCreateImage(inst->rend.device, &imageInfo, nullptr, &vpInst->colorImage);
        CORE_ASSERT(err == VK_SUCCESS && "Image creation failed");

    }

    // Color image memory creation
    {

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(inst->rend.device, vpInst->colorImage, &memRequirements);
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
        VkResult err = vkAllocateMemory(inst->rend.device, &allocInfo, nullptr, &vpInst->colorImageMem);
        CORE_ASSERT(err == VK_SUCCESS && "Buffer allocation failed");

        vkBindImageMemory(inst->rend.device, vpInst->colorImage, vpInst->colorImageMem, 0);

    }

    // Color image view creation
    {

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.image = vpInst->colorImage;
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

        VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &vpInst->colorImageView);
        CORE_ASSERT(err == VK_SUCCESS && "Image view creation failed");

    }

    // Depth image creation
    {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = 0;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = c_vlkn::depthFormat;
        imageInfo.extent = { size.width, size.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = c_vlkn::sampleCount;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = 1;
        imageInfo.pQueueFamilyIndices = &inst->rend.graphicsQueueIndex;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult err = vkCreateImage(inst->rend.device, &imageInfo, nullptr, &vpInst->depthImage);
        CORE_ASSERT(err == VK_SUCCESS && "Depth image creation failed");

    }

    // Image memory creation
    {

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(inst->rend.device, vpInst->depthImage, &memRequirements);
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
        VkResult err = vkAllocateMemory(inst->rend.device, &allocInfo, nullptr, &vpInst->depthImageMem);
        CORE_ASSERT(err == VK_SUCCESS && "Buffer allocation failed");

        vkBindImageMemory(inst->rend.device, vpInst->depthImage, vpInst->depthImageMem, 0);

    }

    // Depth image view creation
    {

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.image = vpInst->depthImage;
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

        VkResult err = vkCreateImageView(inst->rend.device, &createInfo, nullptr, &vpInst->depthImageView);
        CORE_ASSERT(err == VK_SUCCESS && "Depth image view creation failed");

    }

    // Update Descriptor Set
    {

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler     = inst->vpRend.frameSampler;
        imageInfo.imageView   = vpInst->imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeDescriptor{};
        writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptor.dstSet = vpInst->descriptorSet;
        writeDescriptor.dstBinding = 0;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptor.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(inst->rend.device, 1, &writeDescriptor, 0, nullptr);

    }

    // Framebuffer creation
    {

        VkImageView attachments[] = { vpInst->colorImageView, vpInst->depthImageView, vpInst->imageView };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = inst->vpRend.renderPass;
        framebufferInfo.attachmentCount = arraySize(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = size.width;
        framebufferInfo.height = size.height;
        framebufferInfo.layers = 1;

        VkResult err = vkCreateFramebuffer(inst->rend.device, &framebufferInfo, nullptr, &vpInst->framebuffer);
        CORE_ASSERT(err == VK_SUCCESS && "Frambuffer creation failed");

    }

}
bool Core::openMeshFile(Instance* inst, const char* file) {

    scopedTimer(t1, &inst->gui.stats.perfTimes.openFile);
    if (file == nullptr) return false;
    if (file[0] == '\0') return false;
    const char* fileTitle = file;
    const char* i = file;
    for (; *i != '\0'; ++i)
        if (*i == '/' || *i == '\\') fileTitle = i + 1;

    // Early out if window already exists
    for (Gui::ViewportGuiData& vpData : inst->gui.vpDatas) {
        if (strcmp(vpData.objectName.get(), fileTitle) == 0) {
            ImGui::SetWindowFocus(vpData.objectName.get()); 
            return false;
        }
    }

    vkQueueWaitIdle(inst->rend.graphicsQueue);

    std::vector<mload::Vertex> vertices;
    std::vector<uint32_t>      indices;

    bool isTextFormat;
    if (mload::openModel(file, &vertices, &indices, &isTextFormat)) return false;

    Core::VertexIndexBuffersInfo buffsInfo{};
    buffsInfo.vertexData = vertices.data();
    buffsInfo.vertexDataSize = vertices.size() * sizeof mload::Vertex;
    buffsInfo.indexData = indices.data();
    buffsInfo.indexDataSize = indices.size() * sizeof uint32_t;

    inst->vpRend.vpInstances.push_back({});
    Core::ViewportInstance& newVpInstance = inst->vpRend.vpInstances.back();
    Core::createGeometryData(inst, &newVpInstance, &buffsInfo);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = inst->rend.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &inst->vpRend.descriptorSetLayout;

    VkResult err = vkAllocateDescriptorSets(inst->rend.device, &allocInfo, &newVpInstance.descriptorSet);
    CORE_ASSERT(err == VK_SUCCESS && "Descriptor set creation failed");

    inst->gui.vpDatas.push_back({});
    Gui::ViewportGuiData& newVpData = inst->gui.vpDatas.back();
    newVpData.open = true;
    newVpData.model = glm::mat4(1.0f);
    newVpData.framebufferTexID = (ImTextureID)newVpInstance.descriptorSet;

    float farthestReachingVertex = 0.0f;
    for (mload::Vertex& vertex : vertices) {
        glm::vec3 pos(vertex.pos.x, vertex.pos.y, vertex.pos.z);
        newVpData.modelCenter += pos;
        float magnitude = glm::length(pos);
        if (magnitude > farthestReachingVertex) farthestReachingVertex = magnitude;

    }
    newVpData.modelCenter /= (float)vertices.size();
    newVpData.zoomDistance = -2.5f * farthestReachingVertex;
    newVpData.farPlaneClip = -30.0f * newVpData.zoomDistance;
    newVpData.zoomMin = -newVpData.farPlaneClip / 3;

    newVpData.objectName.reset(new char[(int)(i - fileTitle) + 1]);
    strcpy(newVpData.objectName.get(), fileTitle);
    newVpData.triangleCount = (uint32_t)indices.size();
    newVpData.uniqueTriangleCount = (uint32_t)vertices.size();
    newVpData.isTextFormat = isTextFormat;

    return true;

}
void Core::destroyGeometryData(VkDevice device, ViewportInstance* vpInst) {

    vkFreeMemory         (device, vpInst->indexBuffMem,   nullptr);
    vkDestroyBuffer      (device, vpInst->indexBuff,      nullptr);
    vkFreeMemory         (device, vpInst->vertBuffMem,    nullptr);
    vkDestroyBuffer      (device, vpInst->vertBuff,       nullptr);

}
void Core::destroyVpImageResources(VkDevice device, ViewportInstance* vpInst) {

    vkDestroyFramebuffer (device, vpInst->framebuffer,    nullptr);
    vkDestroyImageView   (device, vpInst->depthImageView, nullptr);
    vkFreeMemory         (device, vpInst->depthImageMem,  nullptr);
    vkDestroyImage       (device, vpInst->depthImage,     nullptr);
    vkDestroyImageView   (device, vpInst->colorImageView, nullptr);
    vkFreeMemory         (device, vpInst->colorImageMem,  nullptr);
    vkDestroyImage       (device, vpInst->colorImage,     nullptr);
    vkDestroyImageView   (device, vpInst->imageView,      nullptr);
    vkFreeMemory         (device, vpInst->imageMem,       nullptr);
    vkDestroyImage       (device, vpInst->image,          nullptr);

}


VkResult Core::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void Core::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Core::Callback::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    Instance* inst = (Instance*)GetWindowLongPtr(hwnd, GWLP_USERDATA); 

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) return true;

    switch (uMsg)
    {
    case WM_NCHITTEST: {
        POINT mpos = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &mpos);

        constexpr int borderWidth = 8;
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        // Check if the cursor is in a corner or edge for resizing
        if (
            mpos.y < inst->gui.styleEx.sizes.titleBarHeight && 
            mpos.y >= borderWidth && 
            mpos.x > inst->gui.styleEx.sizes.menuBarEndExtent && 
            mpos.x < windowRect.right - 3 * inst->gui.styleEx.sizes.wndBtnWidth ) {
            return HTCAPTION;
        }
        else if (mpos.y < borderWidth && mpos.x < borderWidth) {
            return HTTOPLEFT;
        }
        else if (mpos.y < borderWidth && mpos.x > windowRect.right - borderWidth) {
            return HTTOPRIGHT;
        }
        else if (mpos.y > windowRect.bottom - borderWidth && mpos.x < borderWidth) {
            return HTBOTTOMLEFT;
        }
        else if (mpos.y > windowRect.bottom - borderWidth && mpos.x > windowRect.right - borderWidth) {
            return HTBOTTOMRIGHT;
        }
        else if (mpos.y < borderWidth) {
            return HTTOP;
        }
        else if (mpos.y > windowRect.bottom - borderWidth) {
            return HTBOTTOM;
        }
        else if (mpos.x < borderWidth) {
            return HTLEFT;
        }
        else if (mpos.x > windowRect.right - borderWidth) {
            return HTRIGHT;
        }
        return HTCLIENT; 
    }
    case WM_SIZE: {

        inst->wind.m_size.x = LOWORD(lParam);
        inst->wind.m_size.y = HIWORD(lParam);

        recreateSwapchain(inst);
        App::render(inst);

        return 0; 
    }
    case WM_ACTIVATE: {
        RECT rcClient;
        GetWindowRect(hwnd, &rcClient);

        SetWindowPos(hwnd,
            NULL,
            rcClient.left, rcClient.top,
            rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
            SWP_FRAMECHANGED
        );

        return 0; 

    }
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = c_minWidth;
        lpMMI->ptMinTrackSize.y = c_minHeight;
        return 0;
    }
    case WM_NCCALCSIZE: {
        if (wParam == TRUE)
        {
            NCCALCSIZE_PARAMS* pncsp = (NCCALCSIZE_PARAMS*)lParam;

            // Offset the 12 pixel extension when maximized
            if (IsZoomed(hwnd)) {
                pncsp->rgrc[0].left += 12;  
                pncsp->rgrc[0].top += 12;   
                pncsp->rgrc[0].right -= 12; 
                pncsp->rgrc[0].bottom -= 12;
            }
            // Extend the border 1 pixel in for the standard border
            else {
                pncsp->rgrc[0].left += 1;
                pncsp->rgrc[0].top += 1;
                pncsp->rgrc[0].right -= 1;
                pncsp->rgrc[0].bottom -= 1;
            }

            return 0;
        }
        break;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);

}
VKAPI_ATTR VkBool32 VKAPI_CALL Core::Callback::validationLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {

    printf("%s\n\n", pCallbackData->pMessage);
    assert(!"Validation layer callback called! Check the console for the error.");

    return VK_FALSE;

}