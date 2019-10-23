#pragma once

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <vulkan/vulkan.h>

#include <vector>

class vk_helper {
public:
    explicit vk_helper() { mIsReady.store(false); }
    void initialize(ANativeWindow* window, AAssetManager* assetManager);
    void drawFrame();
    void destroy();

private:
    void createInstance();
    void createDevice();
    void createSwapchain(ANativeWindow* window);
    void createRenderPass();
    void loadShaderFromFile(AAssetManager* assetManager, const char* filePath,
                            VkShaderModule* outShader);
    void createGraphicsPipeline(AAssetManager* assetManager);
    void createVertexBuffer();
    void createCommandBuffers();
    void createSemaphore(VkSemaphore* semaphore);
    void createSemaphores();
    void createImageView(uint32_t index);
    void createFramebuffer(uint32_t index);
    void recordCommandBuffer(uint32_t index);

    std::atomic<bool> mIsReady;

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
    PFN_vkGetPhysicalDeviceMemoryProperties mGetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR mGetPhysicalDeviceSurfaceCapabilitiesKHR =
            nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR mGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR mGetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties mGetPhysicalDeviceQueueFamilyProperties = nullptr;

    // GET_DEVICE_PROC functions
    PFN_vkAcquireNextImageKHR mAcquireNextImageKHR = nullptr;
    PFN_vkAllocateCommandBuffers mAllocateCommandBuffers = nullptr;
    PFN_vkAllocateMemory mAllocateMemory = nullptr;
    PFN_vkBeginCommandBuffer mBeginCommandBuffer = nullptr;
    PFN_vkCmdBeginRenderPass mCmdBeginRenderPass = nullptr;
    PFN_vkCmdBindVertexBuffers mCmdBindVertexBuffers = nullptr;
    PFN_vkCmdEndRenderPass mCmdEndRenderPass = nullptr;
    PFN_vkCmdBindPipeline mCmdBindPipeline = nullptr;
    PFN_vkCmdDraw mCmdDraw = nullptr;
    PFN_vkCmdPushConstants mCmdPushConstants = nullptr;
    PFN_vkCreateBuffer mCreateBuffer = nullptr;
    PFN_vkCreateCommandPool mCreateCommandPool = nullptr;
    PFN_vkCreateFramebuffer mCreateFramebuffer = nullptr;
    PFN_vkCreateGraphicsPipelines mCreateGraphicsPipelines = nullptr;
    PFN_vkCreateImageView mCreateImageView = nullptr;
    PFN_vkCreatePipelineLayout mCreatePipelineLayout = nullptr;
    PFN_vkCreateRenderPass mCreateRenderPass = nullptr;
    PFN_vkCreateSemaphore mCreateSemaphore = nullptr;
    PFN_vkCreateShaderModule mCreateShaderModule = nullptr;
    PFN_vkCreateSwapchainKHR mCreateSwapchainKHR = nullptr;
    PFN_vkDestroyBuffer mDestroyBuffer = nullptr;
    PFN_vkDestroyCommandPool mDestroyCommandPool = nullptr;
    PFN_vkDestroyDevice mDestroyDevice = nullptr;
    PFN_vkDestroyFramebuffer mDestroyFramebuffer = nullptr;
    PFN_vkDestroyImageView mDestroyImageView = nullptr;
    PFN_vkDestroyPipeline mDestroyPipeline = nullptr;
    PFN_vkDestroyPipelineLayout mDestroyPipelineLayout = nullptr;
    PFN_vkDestroyRenderPass mDestroyRenderPass = nullptr;
    PFN_vkDestroySemaphore mDestroySemaphore = nullptr;
    PFN_vkDestroyShaderModule mDestroyShaderModule = nullptr;
    PFN_vkDestroySwapchainKHR mDestroySwapchainKHR = nullptr;
    PFN_vkEndCommandBuffer mEndCommandBuffer = nullptr;
    PFN_vkFreeCommandBuffers mFreeCommandBuffers = nullptr;
    PFN_vkFreeMemory mFreeMemory = nullptr;
    PFN_vkGetBufferMemoryRequirements mGetBufferMemoryRequirements = nullptr;
    PFN_vkGetDeviceQueue mGetDeviceQueue = nullptr;
    PFN_vkGetSwapchainImagesKHR mGetSwapchainImagesKHR = nullptr;
    PFN_vkQueuePresentKHR mQueuePresentKHR = nullptr;
    PFN_vkQueueSubmit mQueueSubmit = nullptr;
    PFN_vkDeviceWaitIdle mDeviceWaitIdle = nullptr;

    // Members need to be tracked
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
    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mDeviceMemory = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    VkSemaphore mFreeAcquireSemaphore = VK_NULL_HANDLE;
    VkSemaphore mFreeRenderSemaphore = VK_NULL_HANDLE;
    std::vector<VkSemaphore> mAcquireSemaphores;
    std::vector<VkSemaphore> mRenderSemaphores;
};