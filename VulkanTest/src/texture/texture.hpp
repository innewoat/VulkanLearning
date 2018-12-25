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

#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h>

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
struct ImageCreateInfo
{
};

class Application
{
  protected:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkDebugReportCallbackEXT debugReportCallback{};
    uint32_t queueFamilyIndex;
    VkQueue queue;
    VkCommandPool commandPool;

  public:
    ~Application();
    uint32_t getMemoryTypeIndex(uint32_t, VkMemoryPropertyFlags);
    VkResult createBuffer(BufferCreateInfo &);
    void submitWork(VkCommandBuffer, VkQueue);

    void setInstance();
    void setDevice();
    void setTexture();
    void setVertex();
    void setFramebufferAtta();
    void setRenderPass();
    void setPipeline();
    void setCommand();
    void saveImage();
};