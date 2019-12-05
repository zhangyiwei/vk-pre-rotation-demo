#pragma once

#include <android_native_app_glue.h>

#include <glm/glm.hpp>
#include <vector>

#include "VkHelper.h"

class Renderer {
private:
    struct Texture {
        VkSampler sampler;
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        uint32_t width;
        uint32_t height;

        Texture()
              : sampler(VK_NULL_HANDLE),
                image(VK_NULL_HANDLE),
                memory(VK_NULL_HANDLE),
                view(VK_NULL_HANDLE),
                width(0),
                height(0) {}
    };

    struct ConstantBlock {
        glm::vec2 scale;
        glm::mat2 rotate;
    };

public:
    explicit Renderer() {}
    void initialize(ANativeWindow* window, AAssetManager* assetManager);
    void drawFrame();
    void destroy();

private:
    void createInstance();
    void createDevice();
    void createSurface(ANativeWindow* window);
    void createSwapchain(VkSwapchainKHR oldSwapchain);
    uint32_t getMemoryTypeIndex(uint32_t typeBits, VkFlags mask);
    void setImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldImageLayout,
                        VkImageLayout newImageLayout, VkPipelineStageFlags srcStages,
                        VkPipelineStageFlags dstStages);
    void loadTextureFromFile(const char* filePath, Texture* outTexture);
    void createTextures();
    void createDescriptorSet();
    void createRenderPass();
    void loadShaderFromFile(const char* filePath, VkShaderModule* outShader);
    void createGraphicsPipeline();
    void createVertexBuffer();
    void createCommandBuffers();
    void createSemaphore(VkSemaphore* outSemaphore);
    void createSemaphores();
    void createFences();
    void createFramebuffer(uint32_t index);
    void recordCommandBuffer(uint32_t frameIndex, uint32_t imageIndex);
    void destroyOldSwapchain();

    // Helper member for Vulkan entry points
    VkHelper mVk;
    // A pointer to cache AAssetManager
    AAssetManager* mAssetManager = nullptr;

    // Stable baseline members
    VkInstance mInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mGpu = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    uint32_t mQueueFamilyIndex = 0;
    VkQueue mQueue = VK_NULL_HANDLE;

    // Swapchain related members
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR mColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    VkSurfaceTransformFlagBitsKHR mPreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    uint32_t mFrameCount = 0;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    std::vector<VkImage> mImages;
    std::vector<VkImageView> mImageViews;
    std::vector<VkFramebuffer> mFramebuffers;
    // For swapchain recreation
    uint32_t mRetireFrame = 0;
    VkSwapchainKHR mOldSwapchain = VK_NULL_HANDLE;
    std::vector<VkImage> mOldImages;
    std::vector<VkImageView> mOldImageViews;
    std::vector<VkFramebuffer> mOldFramebuffers;

    // Graphics pipeline related members
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;

    // Descriptor related members
    std::vector<Texture> mTextures;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

    // Vertex buffer related members
    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mVertexMemory = VK_NULL_HANDLE;

    // Command buffer related members
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    // Semaphores for synchronization
    std::vector<VkSemaphore> mAcquireSemaphores;
    std::vector<VkSemaphore> mRenderSemaphores;

    // Fences for latency control
    std::vector<VkFence> mInflightFences;

    // App specific constants
    static constexpr const char* kRequiredInstanceExtensions[2] = {
            "VK_KHR_surface",
            "VK_KHR_android_surface",
    };
    static constexpr const char* kRequiredDeviceExtensions[1] = {
            "VK_KHR_swapchain",
    };
    static constexpr const uint32_t kReqImageCount = 3;
    static constexpr const uint32_t kInflight = 2;
    static constexpr const uint32_t kTextureCount = 1;
    static constexpr const char* kTextureFiles[kTextureCount] = {
            "sample_tex.png",
    };
    static constexpr const char* kVertexShaderFile = "texture.vert.spv";
    static constexpr const char* kFragmentShaderFile = "texture.frag.spv";
    static constexpr const uint32_t kLogInterval = 100;
    static constexpr const uint64_t kTimeout30Sec = 30000000000;
};
