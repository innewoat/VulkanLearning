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
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};
struct BufferCreateInfo
{
    VkBufferUsageFlags usageFlags;
    VkMemoryPropertyFlags memoryPropertyFlags;
    VkBuffer &buffer;
    VkDeviceMemory &memory;
    VkDeviceSize size;
    void *data = nullptr;
};
struct ImageCreateInfo
{
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags properties;
    VkImage &image;
    VkDeviceMemory &memory;
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

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    std::vector<Vertex> vertices;

    int32_t width;
    int32_t height;

    VkFramebuffer framebuffer;
    FrameBufferAttachment colorAttachment, depthAttachment;

    VkRenderPass renderPass;

  public:
    ~Application();
    uint32_t getMemoryTypeIndex(uint32_t, VkMemoryPropertyFlags);
    VkResult createBuffer(BufferCreateInfo &);
    VkResult createImage(ImageCreateInfo &);
    VkResult createImageView(VkImage &, VkFormat, VkImageView &);
    VkResult createSampler(VkSampler &);

    void submitWork(VkCommandBuffer &, VkQueue &);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer &, VkQueue &);
    void copyBuffer(VkBuffer &src, VkBuffer &dst, VkDeviceSize size);
    void copyBufferToImage(VkBuffer &src, VkImage &dst, uint32_t width, uint32_t height);
    void transitionImageLayout(VkImage &, VkImageLayout oldLayout, VkImageLayout newLayout);

    void setInstance();
    void setDevice();
    void setTexture();
    void setVertex();
    void setFramebufferAtta();
    void setRenderPass();
    void setDescriptorSetLayout();
    void setPipeline();
    void setDescriptorPool();
    void setDescriptorSets();
    void setCommand();
    void saveImage();
};