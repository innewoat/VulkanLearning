/*
* Some basic structure and data used in a rendering without window application
*/

#ifndef DEFAULT_DATA_HPP
#define DEFAULT_DATA_HPP

#include <vulkan/vulkan.h>
#include <vector>

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

class AppData
{
  public:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t dequeFamilyIndex;
    VkPipelineCache pipelineCache;
    VkQueue queue;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkShaderModule> shaderModules;
    VkBuffer vertexBuffer, indexBuffer;
    VkDeviceMemory vertexMemory, indexMemory;
    int32_t width, height;
    VkFramebuffer framebuffer;
    FrameBufferAttachment colorAttachment, depthAttachment;
    VkRenderPass renderPass;
    VkDebugReportCallbackEXT debugReportCallback{};
};

#endif