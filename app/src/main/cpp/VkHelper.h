#pragma once

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <vulkan/vulkan.h>

#include <vector>

class VkHelper {
private:
    struct Texture {
        VkSampler sampler;
        VkImage image;
        VkImageLayout imageLayout;
        VkDeviceMemory memory;
        VkImageView view;
        uint32_t width;
        uint32_t height;

        Texture()
              : sampler(VK_NULL_HANDLE),
                image(VK_NULL_HANDLE),
                imageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                memory(VK_NULL_HANDLE),
                view(VK_NULL_HANDLE),
                width(0),
                height(0) {}
    };

public:
    explicit VkHelper() { mIsReady.store(false); }
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

    // GET_PROC functions
    PFN_vkCreateInstance mCreateInstance = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties mEnumerateInstanceExtensionProperties = nullptr;
    PFN_vkEnumerateInstanceVersion mEnumerateInstanceVersion = nullptr;

    // GET_INSTANCE_PROC functions
    PFN_vkCreateAndroidSurfaceKHR mCreateAndroidSurfaceKHR = nullptr;
    PFN_vkCreateDevice mCreateDevice = nullptr;
    PFN_vkDestroyInstance mDestroyInstance = nullptr;
    PFN_vkDestroySurfaceKHR mDestroySurfaceKHR = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties mEnumerateDeviceExtensionProperties = nullptr;
    PFN_vkEnumeratePhysicalDevices mEnumeratePhysicalDevices = nullptr;
    PFN_vkGetDeviceProcAddr mGetDeviceProcAddr = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties mGetPhysicalDeviceFormatProperties = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties mGetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR mGetPhysicalDeviceSurfaceCapabilitiesKHR =
            nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR mGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR mGetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties mGetPhysicalDeviceQueueFamilyProperties = nullptr;

    // GET_DEVICE_PROC functions
    PFN_vkAcquireNextImageKHR mAcquireNextImageKHR = nullptr;
    PFN_vkAllocateCommandBuffers mAllocateCommandBuffers = nullptr;
    PFN_vkAllocateDescriptorSets mAllocateDescriptorSets = nullptr;
    PFN_vkAllocateMemory mAllocateMemory = nullptr;
    PFN_vkBeginCommandBuffer mBeginCommandBuffer = nullptr;
    PFN_vkBindBufferMemory mBindBufferMemory = nullptr;
    PFN_vkBindImageMemory mBindImageMemory = nullptr;
    PFN_vkCmdBeginRenderPass mCmdBeginRenderPass = nullptr;
    PFN_vkCmdBindDescriptorSets mCmdBindDescriptorSets = nullptr;
    PFN_vkCmdBindPipeline mCmdBindPipeline = nullptr;
    PFN_vkCmdBindVertexBuffers mCmdBindVertexBuffers = nullptr;
    PFN_vkCmdCopyImage mCmdCopyImage = nullptr;
    PFN_vkCmdDraw mCmdDraw = nullptr;
    PFN_vkCmdEndRenderPass mCmdEndRenderPass = nullptr;
    PFN_vkCmdPipelineBarrier mCmdPipelineBarrier = nullptr;
    PFN_vkCreateBuffer mCreateBuffer = nullptr;
    PFN_vkCreateCommandPool mCreateCommandPool = nullptr;
    PFN_vkCreateDescriptorPool mCreateDescriptorPool = nullptr;
    PFN_vkCreateDescriptorSetLayout mCreateDescriptorSetLayout = nullptr;
    PFN_vkCreateFence mCreateFence = nullptr;
    PFN_vkCreateFramebuffer mCreateFramebuffer = nullptr;
    PFN_vkCreateGraphicsPipelines mCreateGraphicsPipelines = nullptr;
    PFN_vkCreateImage mCreateImage = nullptr;
    PFN_vkCreateImageView mCreateImageView = nullptr;
    PFN_vkCreatePipelineLayout mCreatePipelineLayout = nullptr;
    PFN_vkCreateRenderPass mCreateRenderPass = nullptr;
    PFN_vkCreateSampler mCreateSampler = nullptr;
    PFN_vkCreateSemaphore mCreateSemaphore = nullptr;
    PFN_vkCreateShaderModule mCreateShaderModule = nullptr;
    PFN_vkCreateSwapchainKHR mCreateSwapchainKHR = nullptr;
    PFN_vkDestroyBuffer mDestroyBuffer = nullptr;
    PFN_vkDestroyCommandPool mDestroyCommandPool = nullptr;
    PFN_vkDestroyDescriptorPool mDestroyDescriptorPool = nullptr;
    PFN_vkDestroyDescriptorSetLayout mDestroyDescriptorSetLayout = nullptr;
    PFN_vkDestroyDevice mDestroyDevice = nullptr;
    PFN_vkDestroyFence mDestroyFence = nullptr;
    PFN_vkDestroyFramebuffer mDestroyFramebuffer = nullptr;
    PFN_vkDestroyImage mDestroyImage = nullptr;
    PFN_vkDestroyImageView mDestroyImageView = nullptr;
    PFN_vkDestroyPipeline mDestroyPipeline = nullptr;
    PFN_vkDestroyPipelineLayout mDestroyPipelineLayout = nullptr;
    PFN_vkDestroyRenderPass mDestroyRenderPass = nullptr;
    PFN_vkDestroySampler mDestroySampler = nullptr;
    PFN_vkDestroySemaphore mDestroySemaphore = nullptr;
    PFN_vkDestroyShaderModule mDestroyShaderModule = nullptr;
    PFN_vkDestroySwapchainKHR mDestroySwapchainKHR = nullptr;
    PFN_vkDeviceWaitIdle mDeviceWaitIdle = nullptr;
    PFN_vkEndCommandBuffer mEndCommandBuffer = nullptr;
    PFN_vkFreeCommandBuffers mFreeCommandBuffers = nullptr;
    PFN_vkFreeDescriptorSets mFreeDescriptorSets = nullptr;
    PFN_vkFreeMemory mFreeMemory = nullptr;
    PFN_vkGetBufferMemoryRequirements mGetBufferMemoryRequirements = nullptr;
    PFN_vkGetDeviceQueue mGetDeviceQueue = nullptr;
    PFN_vkGetImageMemoryRequirements mGetImageMemoryRequirements = nullptr;
    PFN_vkGetImageSubresourceLayout mGetImageSubresourceLayout = nullptr;
    PFN_vkGetSwapchainImagesKHR mGetSwapchainImagesKHR = nullptr;
    PFN_vkMapMemory mMapMemory = nullptr;
    PFN_vkQueuePresentKHR mQueuePresentKHR = nullptr;
    PFN_vkQueueSubmit mQueueSubmit = nullptr;
    PFN_vkUnmapMemory mUnmapMemory = nullptr;
    PFN_vkUpdateDescriptorSets mUpdateDescriptorSets = nullptr;
    PFN_vkWaitForFences mWaitForFences = nullptr;

    // Members need to be tracked
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
