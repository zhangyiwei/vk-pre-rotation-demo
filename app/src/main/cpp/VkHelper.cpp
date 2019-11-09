#include "VkHelper.h"

#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Utils.h"

#define GET_PROC(F) \
    m##F = reinterpret_cast<PFN_vk##F>(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vk" #F))
#define GET_INST_PROC(F) \
    m##F = reinterpret_cast<PFN_vk##F>(vkGetInstanceProcAddr(mInstance, "vk" #F))
#define GET_DEV_PROC(F) m##F = reinterpret_cast<PFN_vk##F>(mGetDeviceProcAddr(mDevice, "vk" #F))

/* public APIs start here */
void VkHelper::initialize(ANativeWindow* window, AAssetManager* assetManager) {
    ASSERT(assetManager);
    mAssetManager = assetManager;

    createInstance();
    createDevice();
    createSwapchain(window);
    createTextures();
    createDescriptorSet();
    createRenderPass();
    createGraphicsPipeline();
    createVertexBuffer();
    createCommandBuffers();
    createSemaphores();

    mIsReady.store(true);
}

void VkHelper::drawFrame() {
    if (!mIsReady.load()) {
        ALOGD("Vulkan is not ready yet");
        return;
    }

    VkSemaphore currentAcquireSemaphore = mFreeAcquireSemaphore;
    VkSemaphore currentRenderSemaphore = mFreeRenderSemaphore;

    uint32_t index;
    ASSERT(mAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, currentAcquireSemaphore,
                                VK_NULL_HANDLE, &index) == VK_SUCCESS);

    if (mFramebuffers[index] == VK_NULL_HANDLE) {
        createFramebuffer(index);
    }

    recordCommandBuffer(index);

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &currentAcquireSemaphore,
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &mCommandBuffers[index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &currentRenderSemaphore,
    };
    ASSERT(mQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE) == VK_SUCCESS);

    const VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &currentRenderSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &mSwapchain,
            .pImageIndices = &index,
            .pResults = nullptr,
    };
    VkResult ret = mQueuePresentKHR(mQueue, &presentInfo);
    ASSERT(ret == VK_SUCCESS || ret == VK_SUBOPTIMAL_KHR);

    mFreeAcquireSemaphore = mAcquireSemaphores[index];
    mAcquireSemaphores[index] = currentAcquireSemaphore;

    mFreeRenderSemaphore = mRenderSemaphores[index];
    mRenderSemaphores[index] = currentRenderSemaphore;

    // ALOGD("Successfully draw a frame[SUBOPTIMAL(%u)]", ret == VK_SUBOPTIMAL_KHR);
}

void VkHelper::destroy() {
    if (mDevice != VK_NULL_HANDLE) {
        mDeviceWaitIdle(mDevice);

        for (auto& imageView : mImageViews) {
            mDestroyImageView(mDevice, imageView, nullptr);
        }
        mImageViews.clear();

        for (auto& framebuffer : mFramebuffers) {
            mDestroyFramebuffer(mDevice, framebuffer, nullptr);
        }
        mFramebuffers.clear();

        mDestroySemaphore(mDevice, mFreeAcquireSemaphore, nullptr);
        mFreeAcquireSemaphore = VK_NULL_HANDLE;
        for (auto& semaphore : mAcquireSemaphores) {
            mDestroySemaphore(mDevice, semaphore, nullptr);
        }

        mDestroySemaphore(mDevice, mFreeRenderSemaphore, nullptr);
        mFreeRenderSemaphore = VK_NULL_HANDLE;
        for (auto& semaphore : mRenderSemaphores) {
            mDestroySemaphore(mDevice, semaphore, nullptr);
        }

        if (!mCommandBuffers.empty()) {
            mFreeCommandBuffers(mDevice, mCommandPool, mCommandBuffers.size(),
                                mCommandBuffers.data());
        }
        mCommandBuffers.clear();
        mDestroyCommandPool(mDevice, mCommandPool, nullptr);
        mCommandPool = VK_NULL_HANDLE;

        mDestroyBuffer(mDevice, mVertexBuffer, nullptr);
        mVertexBuffer = VK_NULL_HANDLE;
        mFreeMemory(mDevice, mDeviceMemory, nullptr);
        mDeviceMemory = VK_NULL_HANDLE;

        mDestroyPipeline(mDevice, mPipeline, nullptr);
        mPipeline = VK_NULL_HANDLE;
        mDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        mPipelineLayout = VK_NULL_HANDLE;

        mDestroyRenderPass(mDevice, mRenderPass, nullptr);
        mRenderPass = VK_NULL_HANDLE;

        mFreeDescriptorSets(mDevice, mDescriptorPool, 1, &mDescriptorSet);
        mDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
        mDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);

        for (auto& texture : mTextures) {
            mDestroyImageView(mDevice, texture.view, nullptr);
            mDestroySampler(mDevice, texture.sampler, nullptr);
            mDestroyImage(mDevice, texture.image, nullptr);
            mFreeMemory(mDevice, texture.memory, nullptr);
        }
        mTextures.clear();

        mDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

        mDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }

    if (mInstance) {
        mDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mSurface = VK_NULL_HANDLE;

        mDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }
}

/* private APIs start here */
static bool hasExtension(const char* extension_name,
                         const std::vector<VkExtensionProperties>& extensions) {
    return std::find_if(extensions.cbegin(), extensions.cend(),
                        [extension_name](const VkExtensionProperties& extension) {
                            return strcmp(extension.extensionName, extension_name) == 0;
                        }) != extensions.cend();
}

void VkHelper::createInstance() {
    GET_PROC(CreateInstance);
    GET_PROC(EnumerateInstanceExtensionProperties);
    GET_PROC(EnumerateInstanceVersion);

    uint32_t instanceVersion = 0;
    ASSERT(mEnumerateInstanceVersion(&instanceVersion) == VK_SUCCESS);
    ASSERT(instanceVersion >= VK_MAKE_VERSION(1, 1, 0));

    uint32_t extensionCount = 0;
    ASSERT(mEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) == VK_SUCCESS);
    std::vector<VkExtensionProperties> supportedInstanceExtensions(extensionCount);
    ASSERT(mEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                 supportedInstanceExtensions.data()) == VK_SUCCESS);

    std::vector<const char*> enabledInstanceExtensions;
    for (const auto extension : kRequiredInstanceExtensions) {
        ASSERT(hasExtension(extension, supportedInstanceExtensions));
        enabledInstanceExtensions.push_back(extension);
    }

    const VkApplicationInfo applicationInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = LOG_TAG,
            .applicationVersion = 0,
            .pEngineName = nullptr,
            .engineVersion = 0,
            .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };
    const VkInstanceCreateInfo instanceInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size()),
            .ppEnabledExtensionNames = enabledInstanceExtensions.data(),
    };

    ASSERT(mCreateInstance(&instanceInfo, nullptr, &mInstance) == VK_SUCCESS);

    GET_INST_PROC(CreateAndroidSurfaceKHR);
    GET_INST_PROC(CreateDevice);
    GET_INST_PROC(DestroyInstance);
    GET_INST_PROC(DestroySurfaceKHR);
    GET_INST_PROC(EnumerateDeviceExtensionProperties);
    GET_INST_PROC(EnumeratePhysicalDevices);
    GET_INST_PROC(GetDeviceProcAddr);
    GET_INST_PROC(GetPhysicalDeviceMemoryProperties);
    GET_INST_PROC(GetPhysicalDeviceFormatProperties);
    GET_INST_PROC(GetPhysicalDeviceQueueFamilyProperties);
    GET_INST_PROC(GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INST_PROC(GetPhysicalDeviceSurfaceSupportKHR);
    GET_INST_PROC(GetPhysicalDeviceSurfaceCapabilitiesKHR);

    ALOGD("Successfully created instance");
}

void VkHelper::createDevice() {
    uint32_t gpuCount = 0;
    ASSERT(mEnumeratePhysicalDevices(mInstance, &gpuCount, nullptr) == VK_SUCCESS);
    ASSERT(gpuCount);
    ALOGD("gpuCount = %u", gpuCount);
    std::vector<VkPhysicalDevice> gpus(gpuCount, VK_NULL_HANDLE);
    ASSERT(mEnumeratePhysicalDevices(mInstance, &gpuCount, gpus.data()) == VK_SUCCESS);
    mGpu = gpus[0];

    uint32_t extensionCount = 0;
    ASSERT(mEnumerateDeviceExtensionProperties(mGpu, nullptr, &extensionCount, nullptr) ==
           VK_SUCCESS);
    std::vector<VkExtensionProperties> supportedDeviceExtensions(extensionCount);
    ASSERT(mEnumerateDeviceExtensionProperties(mGpu, nullptr, &extensionCount,
                                               supportedDeviceExtensions.data()) == VK_SUCCESS);

    std::vector<const char*> enabledDeviceExtensions;
    for (const auto extension : kRequiredDeviceExtensions) {
        ASSERT(hasExtension(extension, supportedDeviceExtensions));
        enabledDeviceExtensions.push_back(extension);
    }

    uint32_t queueFamilyCount = 0;
    mGetPhysicalDeviceQueueFamilyProperties(mGpu, &queueFamilyCount, nullptr);
    ASSERT(queueFamilyCount);
    ALOGD("queueFamilyCount = %u", queueFamilyCount);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    mGetPhysicalDeviceQueueFamilyProperties(mGpu, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t queueFamilyIndex;
    for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex) {
        if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    ASSERT(queueFamilyIndex < queueFamilyCount);
    mQueueFamilyIndex = queueFamilyIndex;
    ALOGD("queueFamilyIndex = %u", queueFamilyIndex);

    const float priority = 1.0F;
    const VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = mQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &priority,
    };
    const VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size()),
            .ppEnabledExtensionNames = enabledDeviceExtensions.data(),
            .pEnabledFeatures = nullptr,
    };
    ASSERT(mCreateDevice(mGpu, &deviceCreateInfo, nullptr, &mDevice) == VK_SUCCESS);

    GET_DEV_PROC(AcquireNextImageKHR);
    GET_DEV_PROC(AllocateCommandBuffers);
    GET_DEV_PROC(AllocateDescriptorSets);
    GET_DEV_PROC(AllocateMemory);
    GET_DEV_PROC(BeginCommandBuffer);
    GET_DEV_PROC(BindBufferMemory);
    GET_DEV_PROC(BindImageMemory);
    GET_DEV_PROC(CmdBeginRenderPass);
    GET_DEV_PROC(CmdBindDescriptorSets);
    GET_DEV_PROC(CmdBindPipeline);
    GET_DEV_PROC(CmdBindVertexBuffers);
    GET_DEV_PROC(CmdCopyImage);
    GET_DEV_PROC(CmdDraw);
    GET_DEV_PROC(CmdEndRenderPass);
    GET_DEV_PROC(CmdPipelineBarrier);
    GET_DEV_PROC(CreateBuffer);
    GET_DEV_PROC(CreateCommandPool);
    GET_DEV_PROC(CreateDescriptorPool);
    GET_DEV_PROC(CreateDescriptorSetLayout);
    GET_DEV_PROC(CreateFence);
    GET_DEV_PROC(CreateFramebuffer);
    GET_DEV_PROC(CreateGraphicsPipelines);
    GET_DEV_PROC(CreateImage);
    GET_DEV_PROC(CreateImageView);
    GET_DEV_PROC(CreatePipelineLayout);
    GET_DEV_PROC(CreateRenderPass);
    GET_DEV_PROC(CreateSampler);
    GET_DEV_PROC(CreateSemaphore);
    GET_DEV_PROC(CreateShaderModule);
    GET_DEV_PROC(CreateSwapchainKHR);
    GET_DEV_PROC(DestroyBuffer);
    GET_DEV_PROC(DestroyCommandPool);
    GET_DEV_PROC(DestroyDescriptorPool);
    GET_DEV_PROC(DestroyDescriptorSetLayout);
    GET_DEV_PROC(DestroyDevice);
    GET_DEV_PROC(DestroyFence);
    GET_DEV_PROC(DestroyFramebuffer);
    GET_DEV_PROC(DestroyImage);
    GET_DEV_PROC(DestroyImageView);
    GET_DEV_PROC(DestroyPipeline);
    GET_DEV_PROC(DestroyPipelineLayout);
    GET_DEV_PROC(DestroyRenderPass);
    GET_DEV_PROC(DestroySampler);
    GET_DEV_PROC(DestroySemaphore);
    GET_DEV_PROC(DestroyShaderModule);
    GET_DEV_PROC(DestroySwapchainKHR);
    GET_DEV_PROC(DeviceWaitIdle);
    GET_DEV_PROC(EndCommandBuffer);
    GET_DEV_PROC(FreeCommandBuffers);
    GET_DEV_PROC(FreeDescriptorSets);
    GET_DEV_PROC(FreeMemory);
    GET_DEV_PROC(GetBufferMemoryRequirements);
    GET_DEV_PROC(GetDeviceQueue);
    GET_DEV_PROC(GetImageMemoryRequirements);
    GET_DEV_PROC(GetImageSubresourceLayout);
    GET_DEV_PROC(GetSwapchainImagesKHR);
    GET_DEV_PROC(MapMemory);
    GET_DEV_PROC(QueuePresentKHR);
    GET_DEV_PROC(QueueSubmit);
    GET_DEV_PROC(UnmapMemory);
    GET_DEV_PROC(UpdateDescriptorSets);
    GET_DEV_PROC(WaitForFences);

    mGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);

    ALOGD("Successfully created device");
}

void VkHelper::createSwapchain(ANativeWindow* window) {
    ASSERT(window);

    const VkAndroidSurfaceCreateInfoKHR surfaceInfo = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = window,
    };
    ASSERT(mCreateAndroidSurfaceKHR(mInstance, &surfaceInfo, nullptr, &mSurface) == VK_SUCCESS);

    VkBool32 surfaceSupported = VK_FALSE;
    ASSERT(mGetPhysicalDeviceSurfaceSupportKHR(mGpu, mQueueFamilyIndex, mSurface,
                                               &surfaceSupported) == VK_SUCCESS);
    ASSERT(surfaceSupported == VK_TRUE);

    uint32_t formatCount = 0;
    ASSERT(mGetPhysicalDeviceSurfaceFormatsKHR(mGpu, mSurface, &formatCount, nullptr) ==
           VK_SUCCESS);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    ASSERT(mGetPhysicalDeviceSurfaceFormatsKHR(mGpu, mSurface, &formatCount, formats.data()) ==
           VK_SUCCESS);

    uint32_t formatIndex;
    for (formatIndex = 0; formatIndex < formatCount; ++formatIndex) {
        if (formats[formatIndex].format == VK_FORMAT_R8G8B8A8_UNORM) {
            break;
        }
    }
    ASSERT(formatIndex < formatCount);
    mFormat = formats[formatIndex].format;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    ASSERT(mGetPhysicalDeviceSurfaceCapabilitiesKHR(mGpu, mSurface, &surfaceCapabilities) ==
           VK_SUCCESS);

    ALOGD("Vulkan Surface Capabilities:\n");
    ALOGD("\timage count: %u - %u\n", surfaceCapabilities.minImageCount,
          surfaceCapabilities.maxImageCount);
    ALOGD("\tarray layers: %u\n", surfaceCapabilities.maxImageArrayLayers);
    ALOGD("\timage size (now): %dx%d\n", surfaceCapabilities.currentExtent.width,
          surfaceCapabilities.currentExtent.height);
    ALOGD("\timage size (extent): %dx%d - %dx%d\n", surfaceCapabilities.minImageExtent.width,
          surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.width,
          surfaceCapabilities.maxImageExtent.height);
    ALOGD("\tusage: %x\n", surfaceCapabilities.supportedUsageFlags);
    ALOGD("\tcurrent transform: %u\n", surfaceCapabilities.currentTransform);
    ALOGD("\tallowed transforms: %x\n", surfaceCapabilities.supportedTransforms);
    ALOGD("\tcomposite alpha flags: %u\n", surfaceCapabilities.supportedCompositeAlpha);

    mWidth = surfaceCapabilities.currentExtent.width;
    mHeight = surfaceCapabilities.currentExtent.height;
    mPreTransform = surfaceCapabilities.currentTransform;

    if ((mPreTransform &
         (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR |
          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR |
          VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR)) != 0) {
        mWidth = mWidth ^ mHeight;
        mHeight = mHeight ^ mWidth;
        mWidth = mWidth ^ mHeight;
    }

    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = mSurface,
            .minImageCount = kReqImageCount,
            .imageFormat = mFormat,
            .imageColorSpace = formats[formatIndex].colorSpace,
            .imageExtent =
                    {
                            .width = mWidth,
                            .height = mHeight,
                    },
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &mQueueFamilyIndex,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_FALSE,
            .oldSwapchain = VK_NULL_HANDLE,
    };
    ASSERT(mCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain) == VK_SUCCESS);

    ASSERT(mGetSwapchainImagesKHR(mDevice, mSwapchain, &mImageCount, nullptr) == VK_SUCCESS);
    ALOGD("Swapchain image count = %u", mImageCount);

    mImages.resize(mImageCount, VK_NULL_HANDLE);
    ASSERT(mGetSwapchainImagesKHR(mDevice, mSwapchain, &mImageCount, mImages.data()) == VK_SUCCESS);

    mImageViews.resize(mImageCount, VK_NULL_HANDLE);
    mFramebuffers.resize(mImageCount, VK_NULL_HANDLE);

    ALOGD("Successfully created swapchain");
}

static std::vector<char> readFileFromAsset(AAssetManager* assetManager, const char* filePath,
                                           int mode) {
    ASSERT(filePath);

    AAsset* file = AAssetManager_open(assetManager, filePath, mode);
    ASSERT(file);

    auto fileLength = (size_t)AAsset_getLength(file);
    std::vector<char> fileContent(fileLength);
    AAsset_read(file, fileContent.data(), fileLength);
    AAsset_close(file);

    return fileContent;
}

uint32_t VkHelper::getMemoryTypeIndex(uint32_t typeBits, VkFlags mask) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    mGetPhysicalDeviceMemoryProperties(mGpu, &memoryProperties);

    for (uint32_t typeIndex = 0; typeIndex < std::numeric_limits<uint32_t>::digits; typeIndex++) {
        if ((typeBits & 1U) == 1 &&
            (memoryProperties.memoryTypes[typeIndex].propertyFlags & mask) == mask) {
            return typeIndex;
        }
        typeBits >>= 1U;
    }
    ASSERT(false);
}

void VkHelper::setImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                              VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                              VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask) {
    VkImageMemoryBarrier imageMemoryBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = oldImageLayout,
            .newLayout = newImageLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange =
                    {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                    },
    };

    switch (oldImageLayout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        default:
            break;
    }

    switch (newImageLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        default:
            break;
    }

    mCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1,
                        &imageMemoryBarrier);

    ALOGD("Successfully transferred image layout from %u to %u", oldImageLayout, newImageLayout);
}

void VkHelper::loadTextureFromFile(const char* filePath, Texture* outTexture) {
    std::vector<char> file = readFileFromAsset(mAssetManager, filePath, AASSET_MODE_BUFFER);
    ASSERT(!file.empty());

    bool needBlit = true;
    VkFormatProperties formatProperties;
    mGetPhysicalDeviceFormatProperties(mGpu, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
    ASSERT((formatProperties.linearTilingFeatures | formatProperties.optimalTilingFeatures) &
           VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
        needBlit = false;
    }

    uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;
    uint32_t channel = 0;
    uint8_t* imageData =
            stbi_load_from_memory((const stbi_uc*)file.data(), file.size(),
                                  reinterpret_cast<int*>(&imageWidth),
                                  reinterpret_cast<int*>(&imageHeight),
                                  reinterpret_cast<int*>(&channel), 4 /*desired_channels*/);
    ASSERT(channel == 4);

    VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .extent =
                    {
                            .width = imageWidth,
                            .height = imageHeight,
                            .depth = 1,
                    },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = (needBlit ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_SAMPLED_BIT),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &mQueueFamilyIndex,
            .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
    };
    ASSERT(mCreateImage(mDevice, &imageCreateInfo, nullptr, &outTexture->image) == VK_SUCCESS);

    VkMemoryRequirements memoryRequirements;
    mGetImageMemoryRequirements(mDevice, outTexture->image, &memoryRequirements);

    uint32_t typeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    VkMemoryAllocateInfo memoryAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = typeIndex,
    };
    ASSERT(mAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &outTexture->memory) ==
           VK_SUCCESS);
    ASSERT(mBindImageMemory(mDevice, outTexture->image, outTexture->memory, 0) == VK_SUCCESS);

    const VkImageSubresource imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
    };
    VkSubresourceLayout subresourceLayout;
    mGetImageSubresourceLayout(mDevice, outTexture->image, &imageSubresource, &subresourceLayout);

    void* textureData;
    ASSERT(mMapMemory(mDevice, outTexture->memory, 0, memoryAllocateInfo.allocationSize, 0,
                      &textureData) == VK_SUCCESS);

    for (uint32_t row = 0, srcPos = 0, cols = 4 * imageWidth; row < imageHeight; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            ((uint8_t*)textureData)[col] = imageData[srcPos++];
        }
        textureData = (uint8_t*)textureData + subresourceLayout.rowPitch;
    }

    mUnmapMemory(mDevice, outTexture->memory);
    stbi_image_free(imageData);
    file.clear();

    outTexture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkCommandPoolCreateInfo commandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = mQueueFamilyIndex,
    };
    VkCommandPool commandPool;
    ASSERT(mCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &commandPool) ==
           VK_SUCCESS);

    VkCommandBuffer commandBuffer;
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    ASSERT(mAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &commandBuffer) ==
           VK_SUCCESS);
    VkCommandBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
    };
    ASSERT(mBeginCommandBuffer(commandBuffer, &cmd_buf_info) == VK_SUCCESS);

    // If linear is supported, we are done
    VkImage stageImage = VK_NULL_HANDLE;
    VkDeviceMemory stageMemory = VK_NULL_HANDLE;
    if (!needBlit) {
        setImageLayout(commandBuffer, outTexture->image, VK_IMAGE_LAYOUT_PREINITIALIZED,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    } else {
        // save current image and mem as staging image and memory
        stageImage = outTexture->image;
        stageMemory = outTexture->memory;
        outTexture->image = VK_NULL_HANDLE;
        outTexture->memory = VK_NULL_HANDLE;

        // Create a tile texture to blit into
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ASSERT(mCreateImage(mDevice, &imageCreateInfo, nullptr, &outTexture->image) == VK_SUCCESS);

        mGetImageMemoryRequirements(mDevice, outTexture->image, &memoryRequirements);

        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex =
                getMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        ASSERT(mAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &outTexture->memory) ==
               VK_SUCCESS);
        ASSERT(mBindImageMemory(mDevice, outTexture->image, outTexture->memory, 0) == VK_SUCCESS);

        // transitions image out of UNDEFINED type
        setImageLayout(commandBuffer, stageImage, VK_IMAGE_LAYOUT_PREINITIALIZED,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT);
        setImageLayout(commandBuffer, outTexture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageCopy bltInfo = {
                .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .srcSubresource.mipLevel = 0,
                .srcSubresource.baseArrayLayer = 0,
                .srcSubresource.layerCount = 1,
                .srcOffset.x = 0,
                .srcOffset.y = 0,
                .srcOffset.z = 0,
                .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .dstSubresource.mipLevel = 0,
                .dstSubresource.baseArrayLayer = 0,
                .dstSubresource.layerCount = 1,
                .dstOffset.x = 0,
                .dstOffset.y = 0,
                .dstOffset.z = 0,
                .extent.width = imageWidth,
                .extent.height = imageHeight,
                .extent.depth = 1,
        };
        mCmdCopyImage(commandBuffer, stageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      outTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bltInfo);

        setImageLayout(commandBuffer, outTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    ASSERT(mEndCommandBuffer(commandBuffer) == VK_SUCCESS);
    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    VkFence fence;
    ASSERT(mCreateFence(mDevice, &fenceInfo, nullptr, &fence) == VK_SUCCESS);

    VkSubmitInfo submitInfo = {
            .pNext = nullptr,
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
    };
    ASSERT(mQueueSubmit(mQueue, 1, &submitInfo, fence) == VK_SUCCESS);
    ASSERT(mWaitForFences(mDevice, 1, &fence, VK_TRUE, 100000000) == VK_SUCCESS);
    mDestroyFence(mDevice, fence, nullptr);

    mFreeCommandBuffers(mDevice, commandPool, 1, &commandBuffer);
    mDestroyCommandPool(mDevice, commandPool, nullptr);
    if (stageImage != VK_NULL_HANDLE) {
        mDestroyImage(mDevice, stageImage, nullptr);
        mFreeMemory(mDevice, stageMemory, nullptr);
    }

    // record the image's original dimensions so we can respect it later
    outTexture->width = imageWidth;
    outTexture->height = imageHeight;

    ALOGD("Successfully loaded texture from %s", filePath);
}

void VkHelper::createTextures() {
    mTextures.resize(kTextureCount);
    for (uint32_t i = 0; i < kTextureCount; i++) {
        loadTextureFromFile(kTextureFiles[i], &mTextures[i]);

        const VkSamplerCreateInfo samplerCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .mipLodBias = 0.0F,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 1,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_NEVER,
                .minLod = 0.0F,
                .maxLod = 0.0F,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                .unnormalizedCoordinates = VK_FALSE,
        };
        ASSERT(mCreateSampler(mDevice, &samplerCreateInfo, nullptr, &mTextures[i].sampler) ==
               VK_SUCCESS);

        VkImageViewCreateInfo viewCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = mTextures[i].image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .components =
                        {
                                VK_COMPONENT_SWIZZLE_R,
                                VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A,
                        },
                .subresourceRange =
                        {
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                0,
                                1,
                                0,
                                1,
                        },
        };
        ASSERT(mCreateImageView(mDevice, &viewCreateInfo, nullptr, &mTextures[i].view) ==
               VK_SUCCESS);
    }

    ALOGD("Successfully created textures");
}

void VkHelper::createDescriptorSet() {
    const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = kTextureCount,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .bindingCount = 1,
            .pBindings = &descriptorSetLayoutBinding,
    };
    ASSERT(mCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr,
                                      &mDescriptorSetLayout) == VK_SUCCESS);

    const VkDescriptorPoolSize descriptorPoolSize = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = kTextureCount,
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &descriptorPoolSize,
    };

    ASSERT(mCreateDescriptorPool(mDevice, &descriptor_pool, nullptr, &mDescriptorPool) ==
           VK_SUCCESS);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = mDescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &mDescriptorSetLayout,
    };
    ASSERT(mAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &mDescriptorSet) ==
           VK_SUCCESS);

    VkDescriptorImageInfo descriptorImageInfo[kTextureCount];
    for (uint32_t i = 0; i < kTextureCount; i++) {
        descriptorImageInfo[i].sampler = mTextures[i].sampler;
        descriptorImageInfo[i].imageView = mTextures[i].view;
        descriptorImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    VkWriteDescriptorSet writeDescriptorSet = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = mDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = kTextureCount,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = descriptorImageInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
    };
    mUpdateDescriptorSets(mDevice, 1, &writeDescriptorSet, 0, nullptr);

    ALOGD("Successfully created descriptor set");
}

void VkHelper::loadShaderFromFile(const char* filePath, VkShaderModule* outShader) {
    ASSERT(filePath);

    std::vector<char> file = readFileFromAsset(mAssetManager, filePath, AASSET_MODE_BUFFER);

    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = file.size(),
            .pCode = (const uint32_t*)(file.data()),
    };
    ASSERT(mCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, outShader) == VK_SUCCESS);

    ALOGD("Successfully created shader module from %s", filePath);
}

void VkHelper::createRenderPass() {
    const VkAttachmentDescription attachmentDescription = {
            .flags = 0,
            .format = mFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    const VkAttachmentReference attachmentReference = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpassDescription = {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentReference,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
    };
    const VkRenderPassCreateInfo renderPassCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescription,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 0,
            .pDependencies = nullptr,
    };
    ASSERT(mCreateRenderPass(mDevice, &renderPassCreateInfo, nullptr, &mRenderPass) == VK_SUCCESS);

    ALOGD("Successfully created render pass");
}

void VkHelper::createGraphicsPipeline() {
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &mDescriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };
    ASSERT(mCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout) ==
           VK_SUCCESS);

    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;
    loadShaderFromFile(kVertexShaderFile, &vertexShader);
    loadShaderFromFile(kFragmentShaderFile, &fragmentShader);

    const VkPipelineShaderStageCreateInfo shaderStages[2] = {
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
            },
    };
    const VkViewport viewports = {
            .x = 0.0F,
            .y = 0.0F,
            .width = (float)mWidth,
            .height = (float)mHeight,
            .minDepth = 0.0F,
            .maxDepth = 1.0F,
    };
    const VkRect2D scissor = {
            .offset =
                    {
                            .x = 0,
                            .y = 0,
                    },
            .extent =
                    {
                            .width = mWidth,
                            .height = mHeight,
                    },
    };
    const VkPipelineViewportStateCreateInfo viewportInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };
    VkSampleMask sampleMask = ~0U;
    const VkPipelineMultisampleStateCreateInfo multisampleInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };
    const VkPipelineColorBlendAttachmentState attachmentStates = {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
            .blendConstants = {0.0F, 0.0F, 0.0F, 0.0F},
    };
    const VkPipelineRasterizationStateCreateInfo rasterInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0,
            .depthBiasClamp = 0,
            .depthBiasSlopeFactor = 0,
            .lineWidth = 1,
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE,
    };
    const VkVertexInputBindingDescription vertexInputBindingDescription = {
            .binding = 0,
            .stride = 5 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] = {
            {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
            },
            {

                    .location = 1,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = sizeof(float) * 3,
            },
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexInputBindingDescription,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = vertexInputAttributeDescriptions,
    };
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = mPipelineLayout,
            .renderPass = mRenderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };
    ASSERT(mCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                    &mPipeline) == VK_SUCCESS);

    mDestroyShaderModule(mDevice, vertexShader, nullptr);
    mDestroyShaderModule(mDevice, fragmentShader, nullptr);

    ALOGD("Successfully created graphics pipeline");
}

void VkHelper::createVertexBuffer() {
    const uint32_t queueFamilyIndex = mQueueFamilyIndex;
    const VkBufferCreateInfo bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = sizeof(vertexData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queueFamilyIndex,
    };
    ASSERT(mCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mVertexBuffer) == VK_SUCCESS);

    VkMemoryRequirements memoryRequirements;
    mGetBufferMemoryRequirements(mDevice, mVertexBuffer, &memoryRequirements);

    uint32_t typeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    VkMemoryAllocateInfo memoryAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = typeIndex,
    };
    ASSERT(mAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mDeviceMemory) == VK_SUCCESS);

    void* data;
    ASSERT(mMapMemory(mDevice, mDeviceMemory, 0, sizeof(vertexData), 0, &data) == VK_SUCCESS);

    memcpy(data, vertexData, sizeof(vertexData));
    mUnmapMemory(mDevice, mDeviceMemory);

    ASSERT(mBindBufferMemory(mDevice, mVertexBuffer, mDeviceMemory, 0) == VK_SUCCESS);

    ALOGD("Successfully created vertex buffer");
}

void VkHelper::createCommandBuffers() {
    const VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = mQueueFamilyIndex,
    };
    ASSERT(mCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mCommandPool) ==
           VK_SUCCESS);

    mCommandBuffers.resize(mImageCount, VK_NULL_HANDLE);
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = mCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = mImageCount,
    };
    ASSERT(mAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, mCommandBuffers.data()) ==
           VK_SUCCESS);

    ALOGD("Successfully created command buffers");
}

void VkHelper::createSemaphore(VkSemaphore* outSemaphore) {
    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    ASSERT(mCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, outSemaphore) == VK_SUCCESS);
}

void VkHelper::createSemaphores() {
    mAcquireSemaphores.resize(mImageCount, VK_NULL_HANDLE);
    mRenderSemaphores.resize(mImageCount, VK_NULL_HANDLE);
    for (int i = 0; i < mImageCount; ++i) {
        createSemaphore(&mAcquireSemaphores[i]);
        createSemaphore(&mRenderSemaphores[i]);
    }

    createSemaphore(&mFreeAcquireSemaphore);
    createSemaphore(&mFreeRenderSemaphore);

    ALOGD("Successfully created semaphores");
}

void VkHelper::createImageView(uint32_t index) {
    const VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = mImages[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = mFormat,
            .components =
                    {
                            .r = VK_COMPONENT_SWIZZLE_R,
                            .g = VK_COMPONENT_SWIZZLE_G,
                            .b = VK_COMPONENT_SWIZZLE_B,
                            .a = VK_COMPONENT_SWIZZLE_A,
                    },
            .subresourceRange =
                    {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                    },
    };
    ASSERT(mCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mImageViews[index]) ==
           VK_SUCCESS);

    ALOGD("Successfully created imageView[%u]", index);
}

void VkHelper::createFramebuffer(uint32_t index) {
    if (mImageViews[index] == VK_NULL_HANDLE) {
        createImageView(index);
    }

    const VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = mRenderPass,
            .attachmentCount = 1,
            .pAttachments = &mImageViews[index],
            .width = mWidth,
            .height = mHeight,
            .layers = 1,
    };
    ASSERT(mCreateFramebuffer(mDevice, &framebufferCreateInfo, nullptr, &mFramebuffers[index]) ==
           VK_SUCCESS);

    ALOGD("Successfully created framebuffer[%u]", index);
}

void VkHelper::recordCommandBuffer(uint32_t index) {
    const VkCommandBufferBeginInfo commandBufferBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
    };
    ASSERT(mBeginCommandBuffer(mCommandBuffers[index], &commandBufferBeginInfo) == VK_SUCCESS);

    const VkClearValue clearVals = {
            .color.float32[0] = 0.0F,
            .color.float32[1] = 0.0F,
            .color.float32[2] = 0.0F,
            .color.float32[3] = 1.0F,
    };
    const VkRenderPassBeginInfo renderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = mRenderPass,
            .framebuffer = mFramebuffers[index],
            .renderArea =
                    {
                            .offset =
                                    {
                                            .x = 0,
                                            .y = 0,
                                    },
                            .extent =
                                    {
                                            .width = mWidth,
                                            .height = mHeight,
                                    },
                    },
            .clearValueCount = 1,
            .pClearValues = &clearVals,
    };
    mCmdBeginRenderPass(mCommandBuffers[index], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    mCmdBindPipeline(mCommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    mCmdBindDescriptorSets(mCommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout,
                           0, 1, &mDescriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    mCmdBindVertexBuffers(mCommandBuffers[index], 0, 1, &mVertexBuffer, &offset);

    mCmdDraw(mCommandBuffers[index], 4, 1, 0, 0);

    mCmdEndRenderPass(mCommandBuffers[index]);

    ASSERT(mEndCommandBuffer(mCommandBuffers[index]) == VK_SUCCESS);
}
