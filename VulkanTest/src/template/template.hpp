#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <assert.h>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "tools.hpp"

#define DEBUG (!NDEBUG)

// some complicated structure
struct FrameBufferAttachment
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};
struct Vertex
{
    float position[3];
    float color[3];
};
struct BufferCreateInfo
{
    VkBufferUsageFlags usageFlags;
    VkMemoryPropertyFlags memoryPropertyFlags;
    VkBuffer *buffer;
    VkDeviceMemory *memory;
    VkDeviceSize size;
    void *data = nullptr;
};

// data used in the app
class AppData
{
  public:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t queueFamilyIndex;
    VkPipelineCache pipelineCache;
    VkQueue queue;
    VkCommandPool commandPool;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkShaderModule> shaderModules;
    VkBuffer vertexBuffer, indexBuffer;
    VkDeviceMemory vertexMemory, indexMemory;
    std::vector<Vertex> vertices;
    int32_t width, height;
    VkFramebuffer framebuffer;
    FrameBufferAttachment colorAttachment, depthAttachment;
    VkRenderPass renderPass;

    VkDebugReportCallbackEXT debugReportCallback{};

    ~AppData()
    {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexMemory, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexMemory, nullptr);
        vkDestroyImageView(device, colorAttachment.view, nullptr);
        vkDestroyImage(device, colorAttachment.image, nullptr);
        vkFreeMemory(device, colorAttachment.memory, nullptr);
        vkDestroyImageView(device, depthAttachment.view, nullptr);
        vkDestroyImage(device, depthAttachment.image, nullptr);
        vkFreeMemory(device, depthAttachment.memory, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineCache(device, pipelineCache, nullptr);
        vkDestroyCommandPool(device, commandPool, nullptr);
        for (auto shadermodule : shaderModules)
        {
            vkDestroyShaderModule(device, shadermodule, nullptr);
        }
        vkDestroyDevice(device, nullptr);
#if DEBUG
        if (debugReportCallback)
        {
            PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
            assert(vkDestroyDebugReportCallback);
            vkDestroyDebugReportCallback(instance, debugReportCallback, nullptr);
        }
#endif
        vkDestroyInstance(instance, nullptr);
    }
};

uint32_t getMemoryTypeIndex(AppData &appData, uint32_t typeBits, VkMemoryPropertyFlags properties);
VkResult createBuffer(AppData &appData, BufferCreateInfo &bci);
void submitWork(AppData &appData, VkCommandBuffer cmdBuffer, VkQueue queue);
void setInstance(AppData &appData);
void setDevice(AppData &appData);
void setVertex(AppData &appData);
void setFramebufferAtta(AppData &appData);
void setRenderPass(AppData &appData);
void setPipeline(AppData &appData);
void setCommand(AppData &appData);
void saveImage(AppData &appData);
void buildVertex(std::vector<Vertex> &vertices, std::vector<Vertex> input, int cur, int target);