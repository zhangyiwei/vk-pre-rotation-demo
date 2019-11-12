#pragma once

#include <android_native_app_glue.h>

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

public:
    explicit Renderer() { mIsReady.store(false); }
    void initialize(ANativeWindow* window, AAssetManager* assetManager);
    void drawFrame();
    void destroy();

private:
    void createInstance();
    void createDevice();
    void createSwapchain(ANativeWindow* window);
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
    void createImageView(uint32_t index);
    void createFramebuffer(uint32_t index);
    void recordCommandBuffer(uint32_t index);

    VkHelper mVk;
    std::atomic<bool> mIsReady;
    AAssetManager* mAssetManager = nullptr;

    VkInstance mInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mGpu = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    uint32_t mQueueFamilyIndex = 0;
    VkQueue mQueue = VK_NULL_HANDLE;

    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    VkSurfaceTransformFlagBitsKHR mPreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    uint32_t mImageCount = 0;
    std::vector<VkImage> mImages;
    std::vector<VkImageView> mImageViews;
    std::vector<VkFramebuffer> mFramebuffers;

    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkPipeline mPipeline = VK_NULL_HANDLE;

    std::vector<Texture> mTextures;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mDeviceMemory = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    VkSemaphore mFreeAcquireSemaphore = VK_NULL_HANDLE;
    VkSemaphore mFreeRenderSemaphore = VK_NULL_HANDLE;
    std::vector<VkSemaphore> mAcquireSemaphores;
    std::vector<VkSemaphore> mRenderSemaphores;

    // App specific constants
    static constexpr const char* kRequiredInstanceExtensions[2] = {
            "VK_KHR_surface",
            "VK_KHR_android_surface",
    };
    static constexpr const char* kRequiredDeviceExtensions[1] = {
            "VK_KHR_swapchain",
    };
    static constexpr const uint32_t kReqImageCount = 3;
    static constexpr const uint32_t kTextureCount = 1;
    static constexpr const char* kTextureFiles[kTextureCount] = {
            "sample_tex.png",
    };
    static constexpr const char* kVertexShaderFile = "texture.vert.spv";
    static constexpr const char* kFragmentShaderFile = "texture.frag.spv";
    // clang-format off
    static constexpr const float vertexData[20] = {
            -1.0F, -1.0F, 0.0F, 0.0F, 0.0F, // LT
            -1.0F,  1.0F, 0.0F, 0.0F, 1.0F, // LB
            1.0F, -1.0F, 0.0F, 1.0F, 0.0F, // RT
            1.0F,  1.0F, 0.0F, 1.0F, 1.0F, // RB
    };
    // clang-format on
};