/*
* this is a template project based on Sascha Willems's code
* it will do a rendering without window
*/

#include "template.hpp"

// error handler
static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char *pLayerPrefix,
    const char *pMessage,
    void *pUserData)
{
    printf("[VALIDATION]: %s - %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}

uint32_t getMemoryTypeIndex(AppData &appData, uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(appData.physicalDevice, &deviceMemoryProperties);
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }
    return 0;
}

VkResult createBuffer(AppData &appData, BufferCreateInfo &bci)
{
    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = myvk::initializers::bufferCreateInfo(bci.usageFlags, bci.size);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(appData.device, &bufferCreateInfo, nullptr, bci.buffer));

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = myvk::initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(appData.device, *(bci.buffer), &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(appData, memReqs.memoryTypeBits, bci.memoryPropertyFlags);
    VK_CHECK_RESULT(vkAllocateMemory(appData.device, &memAlloc, nullptr, bci.memory));

    if (bci.data != nullptr)
    {
        void *mapped;
        VK_CHECK_RESULT(vkMapMemory(appData.device, *(bci.memory), 0, bci.size, 0, &mapped));
        memcpy(mapped, bci.data, bci.size);
        vkUnmapMemory(appData.device, *(bci.memory));
    }

    VK_CHECK_RESULT(vkBindBufferMemory(appData.device, *(bci.buffer), *(bci.memory), 0));

    return VK_SUCCESS;
}

// Submit command buffer to a queue and wait for fence until queue operations have been finished
void submitWork(AppData &appData, VkCommandBuffer cmdBuffer, VkQueue queue)
{
    VkSubmitInfo submitInfo = myvk::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    VkFenceCreateInfo fenceInfo = myvk::initializers::fenceCreateInfo();
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(appData.device, &fenceInfo, nullptr, &fence));
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(appData.device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(appData.device, fence, nullptr);
}

void setInstance(AppData &appData)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "example";
    appInfo.pEngineName = "did";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Vulkan instance creation (without surface extensions)
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    uint32_t layerCount = 0;
    const char *validationLayers[] = {"VK_LAYER_LUNARG_standard_validation"};
    layerCount = 1;
#if DEBUG
    // Check if layers are available
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

    bool layersAvailable = true;
    for (auto layerName : validationLayers)
    {
        bool layerAvailable = false;
        for (auto instanceLayer : instanceLayers)
        {
            if (strcmp(instanceLayer.layerName, layerName) == 0)
            {
                layerAvailable = true;
                break;
            }
        }
        if (!layerAvailable)
        {
            layersAvailable = false;
            break;
        }
    }

    if (layersAvailable)
    {
        instanceCreateInfo.ppEnabledLayerNames = validationLayers;
        const char *validationExt = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        instanceCreateInfo.enabledLayerCount = layerCount;
        instanceCreateInfo.enabledExtensionCount = 1;
        instanceCreateInfo.ppEnabledExtensionNames = &validationExt;
    }
#endif
    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &(appData.instance)));
#if DEBUG
    if (layersAvailable)
    {
        VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
        debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debugReportCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugMessageCallback;

        // We have to explicitly load this function.
        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(appData.instance, "vkCreateDebugReportCallbackEXT"));
        assert(vkCreateDebugReportCallbackEXT);
        VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(appData.instance, &debugReportCreateInfo, nullptr, &(appData.debugReportCallback)));
    }
#endif
}

void setDevice(AppData &appData)
{
    uint32_t deviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(appData.instance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(appData.instance, &deviceCount, physicalDevices.data()));
    appData.physicalDevice = physicalDevices[0];

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(appData.physicalDevice, &deviceProperties);
    printf("GPU: %s\n", deviceProperties.deviceName);

    // Request a single graphics queue
    const float defaultQueuePriority(0.0f);
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(appData.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(appData.physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            appData.queueFamilyIndex = i;
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
            break;
        }
    }
    // Create logical device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    VK_CHECK_RESULT(vkCreateDevice(appData.physicalDevice, &deviceCreateInfo, nullptr, &(appData.device)));

    // Get a graphics queue
    vkGetDeviceQueue(appData.device, appData.queueFamilyIndex, 0, &(appData.queue));

    // Command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = appData.queueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(appData.device, &cmdPoolInfo, nullptr, &(appData.commandPool)));
}

void setVertex(AppData &appData)
{
    // build vertex position and color
    {
        int target = 4;

        std::vector<Vertex> input1 = {
            {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        };
        buildVertex(appData.vertices, input1, 0, target);

        std::vector<Vertex> input2 = {
            {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        };
        buildVertex(appData.vertices, input2, 0, target);

        std::vector<Vertex> input3 = {
            {{-0.5f, 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        };
        buildVertex(appData.vertices, input3, 0, target);

        std::vector<Vertex> input4 = {
            {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
            {{0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        };
        buildVertex(appData.vertices, input4, 0, target);
    }

    const VkDeviceSize vertexBufferSize = appData.vertices.size() * sizeof(Vertex);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // Command buffer for copy commands (reused)
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = myvk::initializers::commandBufferAllocateInfo(appData.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer copyCmd;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(appData.device, &cmdBufAllocateInfo, &copyCmd));
    VkCommandBufferBeginInfo cmdBufInfo = myvk::initializers::commandBufferBeginInfo();

    // Copy input data to VRAM using a staging buffer
    {
        BufferCreateInfo bcisrc{
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer,
            &stagingMemory,
            vertexBufferSize,
            appData.vertices.data()};

        // Vertices
        createBuffer(appData, bcisrc);

        BufferCreateInfo bcidest{
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &(appData.vertexBuffer),
            &(appData.vertexMemory),
            vertexBufferSize};
        createBuffer(appData, bcidest);

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
        VkBufferCopy copyRegion = {};
        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, stagingBuffer, appData.vertexBuffer, 1, &copyRegion);
        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        submitWork(appData, copyCmd, appData.queue);

        vkDestroyBuffer(appData.device, stagingBuffer, nullptr);
        vkFreeMemory(appData.device, stagingMemory, nullptr);
    }
}

void setFramebufferAtta(AppData &appData)
{
    appData.width = 1024;
    appData.height = 1024;
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat;
    myvk::tools::getSupportedDepthFormat(appData.physicalDevice, &depthFormat);
    {
        // Color attachment
        VkImageCreateInfo image = myvk::initializers::imageCreateInfo();
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = colorFormat;
        image.extent.width = appData.width;
        image.extent.height = appData.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VkMemoryAllocateInfo memAlloc = myvk::initializers::memoryAllocateInfo();
        VkMemoryRequirements memReqs;

        VK_CHECK_RESULT(vkCreateImage(appData.device, &image, nullptr, &(appData.colorAttachment.image)));
        vkGetImageMemoryRequirements(appData.device, appData.colorAttachment.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = getMemoryTypeIndex(appData, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(appData.device, &memAlloc, nullptr, &(appData.colorAttachment.memory)));
        VK_CHECK_RESULT(vkBindImageMemory(appData.device, appData.colorAttachment.image, appData.colorAttachment.memory, 0));

        VkImageViewCreateInfo colorImageView = myvk::initializers::imageViewCreateInfo();
        colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorImageView.format = colorFormat;
        colorImageView.subresourceRange = {};
        colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageView.subresourceRange.baseMipLevel = 0;
        colorImageView.subresourceRange.levelCount = 1;
        colorImageView.subresourceRange.baseArrayLayer = 0;
        colorImageView.subresourceRange.layerCount = 1;
        colorImageView.image = appData.colorAttachment.image;
        VK_CHECK_RESULT(vkCreateImageView(appData.device, &colorImageView, nullptr, &(appData.colorAttachment.view)));

        // Depth stencil attachment
        image.format = depthFormat;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VK_CHECK_RESULT(vkCreateImage(appData.device, &image, nullptr, &(appData.depthAttachment.image)));
        vkGetImageMemoryRequirements(appData.device, appData.depthAttachment.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = getMemoryTypeIndex(appData, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(appData.device, &memAlloc, nullptr, &(appData.depthAttachment.memory)));
        VK_CHECK_RESULT(vkBindImageMemory(appData.device, appData.depthAttachment.image, appData.depthAttachment.memory, 0));

        VkImageViewCreateInfo depthStencilView = myvk::initializers::imageViewCreateInfo();
        depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthStencilView.format = depthFormat;
        depthStencilView.flags = 0;
        depthStencilView.subresourceRange = {};
        depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        depthStencilView.subresourceRange.baseMipLevel = 0;
        depthStencilView.subresourceRange.levelCount = 1;
        depthStencilView.subresourceRange.baseArrayLayer = 0;
        depthStencilView.subresourceRange.layerCount = 1;
        depthStencilView.image = appData.depthAttachment.image;
        VK_CHECK_RESULT(vkCreateImageView(appData.device, &depthStencilView, nullptr, &(appData.depthAttachment.view)));
    }
}

void setRenderPass(AppData &appData)
{
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat;
    myvk::tools::getSupportedDepthFormat(appData.physicalDevice, &depthFormat);
    std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
    // Color attachment
    attchmentDescriptions[0].format = colorFormat;
    attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    // Depth attachment
    attchmentDescriptions[1].format = depthFormat;
    attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
    renderPassInfo.pAttachments = attchmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();
    VK_CHECK_RESULT(vkCreateRenderPass(appData.device, &renderPassInfo, nullptr, &(appData.renderPass)));

    VkImageView attachments[2];
    attachments[0] = appData.colorAttachment.view;
    attachments[1] = appData.depthAttachment.view;

    VkFramebufferCreateInfo framebufferCreateInfo = myvk::initializers::framebufferCreateInfo();
    framebufferCreateInfo.renderPass = appData.renderPass;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = appData.width;
    framebufferCreateInfo.height = appData.height;
    framebufferCreateInfo.layers = 1;
    VK_CHECK_RESULT(vkCreateFramebuffer(appData.device, &framebufferCreateInfo, nullptr, &(appData.framebuffer)));
}

void setPipeline(AppData &appData)
{
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        myvk::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(appData.device, &descriptorLayout, nullptr, &(appData.descriptorSetLayout)));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        myvk::initializers::pipelineLayoutCreateInfo(nullptr, 0);

    // MVP via push constant block
    VkPushConstantRange pushConstantRange = myvk::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK_RESULT(vkCreatePipelineLayout(appData.device, &pipelineLayoutCreateInfo, nullptr, &(appData.pipelineLayout)));

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(appData.device, &pipelineCacheCreateInfo, nullptr, &(appData.pipelineCache)));

    // Create pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        myvk::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
        myvk::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
        myvk::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        myvk::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        myvk::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportState =
        myvk::initializers::pipelineViewportStateCreateInfo(1, 1);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        myvk::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        myvk::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        myvk::initializers::pipelineCreateInfo(appData.pipelineLayout, appData.renderPass);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    // Vertex bindings an attributes
    // Binding description
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        myvk::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
    };

    // Attribute descriptions
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        myvk::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                 // Position
        myvk::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3), // Color
    };

    VkPipelineVertexInputStateCreateInfo vertexInputState = myvk::initializers::pipelineVertexInputStateCreateInfo();
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    pipelineCreateInfo.pVertexInputState = &vertexInputState;

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName = "main";
    shaderStages[0].module = myvk::tools::loadShader(ASSET_PATH "shaders/template/triangle.vert.spv", appData.device);
    shaderStages[1].module = myvk::tools::loadShader(ASSET_PATH "shaders/template/triangle.frag.spv", appData.device);
    appData.shaderModules = {shaderStages[0].module, shaderStages[1].module};
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(appData.device, appData.pipelineCache, 1, &pipelineCreateInfo, nullptr, &(appData.pipeline)));
}

void setCommand(AppData &appData)
{
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        myvk::initializers::commandBufferAllocateInfo(appData.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VK_CHECK_RESULT(vkAllocateCommandBuffers(appData.device, &cmdBufAllocateInfo, &commandBuffer));

    VkCommandBufferBeginInfo cmdBufInfo =
        myvk::initializers::commandBufferBeginInfo();

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

    VkClearValue clearValues[2];
    clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent.width = appData.width;
    renderPassBeginInfo.renderArea.extent.height = appData.height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.renderPass = appData.renderPass;
    renderPassBeginInfo.framebuffer = appData.framebuffer;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.height = (float)appData.height;
    viewport.width = (float)appData.width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = appData.width;
    scissor.extent.height = appData.height;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appData.pipeline);

    // Render scene
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &appData.vertexBuffer, offsets);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    view = glm::lookAt(
        glm::vec3(0.5f, 2.0f, 2.0f),
        glm::vec3(0.2f, 0.1f, 0.2f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    projection = glm::perspective(glm::radians(45.0f), (float)appData.width / (float)appData.height, 0.1f, 10.0f);
    projection[1][1] = -projection[1][1];

    glm::mat4 mvp = projection * view * model;

    vkCmdPushConstants(commandBuffer, appData.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvp), &mvp);
    vkCmdDraw(commandBuffer, appData.vertices.size(), 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    submitWork(appData, commandBuffer, appData.queue);

    vkDeviceWaitIdle(appData.device);
}

void saveImage(AppData &appData)
{
    const char *imagedata;
    // Create the linear tiled destination image to copy to and to read the memory from
    VkImageCreateInfo imgCreateInfo(myvk::initializers::imageCreateInfo());
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imgCreateInfo.extent.width = appData.width;
    imgCreateInfo.extent.height = appData.height;
    imgCreateInfo.extent.depth = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Create the image
    VkImage dstImage;
    VK_CHECK_RESULT(vkCreateImage(appData.device, &imgCreateInfo, nullptr, &dstImage));
    // Create memory to back up the image
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo(myvk::initializers::memoryAllocateInfo());
    VkDeviceMemory dstImageMemory;
    vkGetImageMemoryRequirements(appData.device, dstImage, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;
    // Memory must be host visible to copy from
    memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(appData, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(appData.device, &memAllocInfo, nullptr, &dstImageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(appData.device, dstImage, dstImageMemory, 0));

    // Do the actual blit from the offscreen image to our host visible destination image
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = myvk::initializers::commandBufferAllocateInfo(appData.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer copyCmd;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(appData.device, &cmdBufAllocateInfo, &copyCmd));
    VkCommandBufferBeginInfo cmdBufInfo = myvk::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

    // Transition destination image to transfer destination layout
    myvk::tools::insertImageMemoryBarrier(
        copyCmd,
        dstImage,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    // colorAttachment.image is already in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be transitioned

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = appData.width;
    imageCopyRegion.extent.height = appData.height;
    imageCopyRegion.extent.depth = 1;

    vkCmdCopyImage(
        copyCmd,
        appData.colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageCopyRegion);

    // Transition destination image to general layout, which is the required layout for mapping the image memory later on
    myvk::tools::insertImageMemoryBarrier(
        copyCmd,
        dstImage,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    submitWork(appData, copyCmd, appData.queue);

    // Get layout of the image (including row pitch)
    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout;

    vkGetImageSubresourceLayout(appData.device, dstImage, &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    vkMapMemory(appData.device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void **)&imagedata);
    imagedata += subResourceLayout.offset;

    /*
			Save host visible framebuffer image to disk (ppm format)
		    */
    const char *filename = "./out/pic/headless.ppm";
    std::ofstream file(filename, std::ios::out | std::ios::binary);

    // ppm header
    file << "P6\n"
         << appData.width << "\n"
         << appData.height << "\n"
         << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    // Check if source is BGR and needs swizzle
    std::vector<VkFormat> formatsBGR = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM};
    const bool colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());

    // ppm binary pixel data
    for (int32_t y = 0; y < appData.height; y++)
    {
        unsigned int *row = (unsigned int *)imagedata;
        for (int32_t x = 0; x < appData.width; x++)
        {
            if (colorSwizzle)
            {
                file.write((char *)row + 2, 1);
                file.write((char *)row + 1, 1);
                file.write((char *)row, 1);
            }
            else
            {
                file.write((char *)row, 3);
            }
            row++;
        }
        imagedata += subResourceLayout.rowPitch;
    }
    file.close();

    printf("Framebuffer image saved to %s\n", filename);

    // Clean up resources
    vkUnmapMemory(appData.device, dstImageMemory);
    vkFreeMemory(appData.device, dstImageMemory, nullptr);
    vkDestroyImage(appData.device, dstImage, nullptr);
}

void buildVertex(std::vector<Vertex> &vertices, std::vector<Vertex> input, int cur, int target)
{
    if (cur > target)
        return;
    int i;
    if (cur == target)
    {
        for (i = 0; i < 3; i++)
        {
            vertices.push_back(input[i]);
        }
    }
    else
    {
        std::vector<Vertex> ninput(input);
        float nposition[3][3];
        for (i = 0; i < 3; i++)
        {
            nposition[i][0] = (input[(i + 1) % 3].position[0] + input[(i + 2) % 3].position[0]) / 2.0f;
            nposition[i][1] = (input[(i + 1) % 3].position[1] + input[(i + 2) % 3].position[1]) / 2.0f;
            nposition[i][2] = (input[(i + 1) % 3].position[2] + input[(i + 2) % 3].position[2]) / 2.0f;
        }
        for (i = 0; i < 3; i++)
        {
            ninput[i].position[0] = input[i].position[0];
            ninput[i].position[1] = input[i].position[1];
            ninput[i].position[2] = input[i].position[2];

            ninput[(i + 1) % 3].position[0] = nposition[(i + 1) % 3][0];
            ninput[(i + 1) % 3].position[1] = nposition[(i + 1) % 3][1];
            ninput[(i + 1) % 3].position[2] = nposition[(i + 1) % 3][2];

            ninput[(i + 2) % 3].position[0] = nposition[(i + 2) % 3][0];
            ninput[(i + 2) % 3].position[1] = nposition[(i + 2) % 3][1];
            ninput[(i + 2) % 3].position[2] = nposition[(i + 2) % 3][2];
            buildVertex(vertices, ninput, cur + 1, target);
        }
    }
}

int main()
{
    printf("Start\n");
    AppData *appData = new AppData();

    setInstance(*appData);
    setDevice(*appData);
    setVertex(*appData);
    setFramebufferAtta(*appData);
    setRenderPass(*appData);
    setPipeline(*appData);
    setCommand(*appData);
    saveImage(*appData);

    vkQueueWaitIdle((*appData).queue);
    printf("End\n");
    getchar();
    delete appData;
    return 0;
}