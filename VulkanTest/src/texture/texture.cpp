#include "texture.hpp"

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

uint32_t Application::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
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

VkResult Application::createBuffer(BufferCreateInfo &bci)
{
    // create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = myvk::initializers::bufferCreateInfo(bci.usageFlags, bci.size);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, bci.buffer));

    // create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = myvk::initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(device, *(bci.buffer), &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, bci.memoryPropertyFlags);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, bci.memory));

    if (bci.data != nullptr)
    {
        void *mapped;
        VK_CHECK_RESULT(vkMapMemory(device, *(bci.memory), 0, bci.size, 0, &mapped));
        memcpy(mapped, bci.data, bci.size);
        vkUnmapMemory(device, *(bci.memory));
    }

    VK_CHECK_RESULT(vkBindBufferMemory(device, *(bci.buffer), *(bci.memory), 0));

    return VK_SUCCESS;
}

// submit command buffer to a queue and wait until finished
void Application::submitWork(VkCommandBuffer cmdBuffer, VkQueue queue)
{
    VkSubmitInfo submitInfo = myvk::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    VkFenceCreateInfo fenceInfo = myvk::initializers::fenceCreateInfo();
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device, fence, nullptr);
}

void Application::setInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "texture";
    appInfo.pEngineName = "did";
    appInfo.apiVersion = VK_API_VERSION_1_1;

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
    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
#if DEBUG
    if (layersAvailable)
    {
        VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
        debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debugReportCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugMessageCallback;

        // We have to explicitly load this function.
        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
        assert(vkCreateDebugReportCallbackEXT);
        VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(instance, &debugReportCreateInfo, nullptr, &debugReportCallback));
    }
#endif
}

void Application::setDevice()
{
    uint32_t deviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));

    // TODO: check if support all extension and layer
    // TODO: choose between GPU cards
    physicalDevice = physicalDevices[0];

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    printf("GPU: %s\n", deviceProperties.deviceName);

    // request a single graphics queue
    const float defaultQueuePriority(0.0f);
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndex = i;
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
    VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    // Get a graphics queue
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // Command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));
}

void Application::setTexture()
{
}

void Application::setVertex()
{
}

void Application::setFramebufferAtta()
{
}

void Application::setRenderPass()
{
}

void Application::setPipeline()
{
}

void Application::setCommand()
{
}

void Application::saveImage()
{
}

Application::~Application()
{
    std::cout << "desc" << std::endl;
}

int main()
{
    Application app;
    return 0;
}