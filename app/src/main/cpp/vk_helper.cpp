#include "vk_helper.h"

#include <android/log.h>
#include <unistd.h>

#include <glm/glm.hpp>

#define LOG_TAG "VKDEMO"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ASSERT(cond)                                                                           \
    if (!(cond)) {                                                                             \
        __android_log_assert(#cond, LOG_TAG, "Error: " #cond " at " __FILE__ ":%d", __LINE__); \
    }

#define GET_PROC(F) \
    m##F = reinterpret_cast<PFN_vk##F>(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vk" #F))
#define GET_INST_PROC(F) \
    m##F = reinterpret_cast<PFN_vk##F>(vkGetInstanceProcAddr(mInstance, "vk" #F))
#define GET_DEV_PROC(F) m##F = reinterpret_cast<PFN_vk##F>(mGetDeviceProcAddr(mDevice, "vk" #F))

static const char* kRequiredInstanceExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_android_surface",
};

static const char* kRequiredDeviceExtensions[] = {
        "VK_KHR_swapchain",
};

// clang-format off
static const float vertexData[] = {
        -1.0F, -1.0F,  0.0F, // LT
        -1.0F,  0.0F,  0.0F, // LC
         0.0F, -1.0F,  0.0F, // CT
         0.0F,  0.0F,  0.0F, // CC
         1.0F, -1.0F,  0.0F, // RT
         1.0F,  0.0F,  0.0F, // RC
        -1.0F,  0.0F,  0.0F, // LC
        -1.0F,  1.0F,  0.0F, // LB
         0.0F,  0.0F,  0.0F, // CC
         0.0F,  1.0F,  0.0F, // CB
         1.0F,  0.0F,  0.0F, // RC
         1.0F,  1.0F,  0.0F, // RB
};

static const float fragData[] = {
        1.0F, 0.0F, 0.0F, // Red
        0.0F, 1.0F, 0.0F, // Green
        0.0F, 0.0F, 1.0F, // Blue
        1.0F, 1.0F, 0.0F, // Yellow
};
// clang-format on

static const uint32_t kReqImageCount = 3;

static bool hasExtension(const char* extension_name,
                         const std::vector<VkExtensionProperties>& extensions) {
    return std::find_if(extensions.cbegin(), extensions.cend(),
                        [extension_name](const VkExtensionProperties& extension) {
                            return strcmp(extension.extensionName, extension_name) == 0;
                        }) != extensions.cend();
}

void vk_helper::createInstance() {
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
    GET_INST_PROC(GetPhysicalDeviceQueueFamilyProperties);
    GET_INST_PROC(GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INST_PROC(GetPhysicalDeviceSurfaceSupportKHR);
    GET_INST_PROC(GetPhysicalDeviceSurfaceCapabilitiesKHR);

    ALOGD("Successfully created instance");
}

void vk_helper::createDevice() {
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
    GET_DEV_PROC(AllocateMemory);
    GET_DEV_PROC(BeginCommandBuffer);
    GET_DEV_PROC(BindBufferMemory);
    GET_DEV_PROC(CmdBeginRenderPass);
    GET_DEV_PROC(CmdBindPipeline);
    GET_DEV_PROC(CmdBindVertexBuffers);
    GET_DEV_PROC(CmdDraw);
    GET_DEV_PROC(CmdEndRenderPass);
    GET_DEV_PROC(CmdPushConstants);
    GET_DEV_PROC(CreateBuffer);
    GET_DEV_PROC(CreateCommandPool);
    GET_DEV_PROC(CreateFramebuffer);
    GET_DEV_PROC(CreateGraphicsPipelines);
    GET_DEV_PROC(CreateImageView);
    GET_DEV_PROC(CreatePipelineLayout);
    GET_DEV_PROC(CreateRenderPass);
    GET_DEV_PROC(CreateSemaphore);
    GET_DEV_PROC(CreateShaderModule);
    GET_DEV_PROC(CreateSwapchainKHR);
    GET_DEV_PROC(DestroyBuffer);
    GET_DEV_PROC(DestroyCommandPool);
    GET_DEV_PROC(DestroyDevice);
    GET_DEV_PROC(DestroyFramebuffer);
    GET_DEV_PROC(DestroyImageView);
    GET_DEV_PROC(DestroyPipeline);
    GET_DEV_PROC(DestroyPipelineLayout);
    GET_DEV_PROC(DestroyRenderPass);
    GET_DEV_PROC(DestroySemaphore);
    GET_DEV_PROC(DestroyShaderModule);
    GET_DEV_PROC(DestroySwapchainKHR);
    GET_DEV_PROC(DeviceWaitIdle);
    GET_DEV_PROC(EndCommandBuffer);
    GET_DEV_PROC(FreeCommandBuffers);
    GET_DEV_PROC(FreeMemory);
    GET_DEV_PROC(GetBufferMemoryRequirements);
    GET_DEV_PROC(GetDeviceQueue);
    GET_DEV_PROC(GetSwapchainImagesKHR);
    GET_DEV_PROC(MapMemory);
    GET_DEV_PROC(QueuePresentKHR);
    GET_DEV_PROC(QueueSubmit);
    GET_DEV_PROC(UnmapMemory);

    mGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);

    ALOGD("Successfully created device");
}

void vk_helper::createSwapchain(ANativeWindow* window) {
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

void vk_helper::createRenderPass() {
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

void vk_helper::loadShaderFromFile(AAssetManager* assetManager, const char* filePath,
                                   VkShaderModule* outShader) {
    ASSERT(filePath);

    AAsset* file = AAssetManager_open(assetManager, filePath, AASSET_MODE_BUFFER);
    ASSERT(file);

    auto fileLength = (size_t)AAsset_getLength(file);
    std::vector<char> fileContent(fileLength);
    AAsset_read(file, fileContent.data(), fileLength);
    AAsset_close(file);

    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = fileLength,
            .pCode = (const uint32_t*)(fileContent.data()),
    };
    ASSERT(mCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, outShader) == VK_SUCCESS);

    ALOGD("Successfully created shader module from %s", filePath);
}

void vk_helper::createGraphicsPipeline(AAssetManager* assetManager) {
    const VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = 3 * sizeof(float),
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };
    ASSERT(mCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout) ==
           VK_SUCCESS);

    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;
    loadShaderFromFile(assetManager, "vert.glsl.spv", &vertexShader);
    loadShaderFromFile(assetManager, "frag.glsl.spv", &fragmentShader);

    const VkPipelineShaderStageCreateInfo shaderStages[2] =
            {{
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
             }};
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
            .srcColorBlendFactor = (VkBlendFactor)0,
            .dstColorBlendFactor = (VkBlendFactor)0,
            .colorBlendOp = (VkBlendOp)0,
            .srcAlphaBlendFactor = (VkBlendFactor)0,
            .dstAlphaBlendFactor = (VkBlendFactor)0,
            .alphaBlendOp = (VkBlendOp)0,
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
            .stride = 3 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    const VkVertexInputAttributeDescription vertexInputAttributeDescription = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexInputBindingDescription,
            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = &vertexInputAttributeDescription,
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

void vk_helper::createVertexBuffer() {
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

    VkPhysicalDeviceMemoryProperties memoryProperties;
    mGetPhysicalDeviceMemoryProperties(mGpu, &memoryProperties);

    uint32_t typeIndex = 0;
    uint32_t typeBits = memoryRequirements.memoryTypeBits;
    for (; typeIndex < std::numeric_limits<uint32_t>::digits; ++typeIndex) {
        if ((typeBits & 1U) == 1) {
            if ((memoryProperties.memoryTypes[typeIndex].propertyFlags &
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                break;
            }
        }
        typeBits >>= 1U;
    }
    ASSERT(typeIndex < 32);

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

void vk_helper::createCommandBuffers() {
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

void vk_helper::createSemaphore(VkSemaphore* semaphore) {
    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    ASSERT(mCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, semaphore) == VK_SUCCESS);
}

void vk_helper::createSemaphores() {
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

void vk_helper::initialize(ANativeWindow* window, AAssetManager* assetManager) {
    createInstance();
    createDevice();
    createSwapchain(window);
    createRenderPass();
    createGraphicsPipeline(assetManager);
    createVertexBuffer();
    createCommandBuffers();
    createSemaphores();
    mIsReady.store(true);
}

void vk_helper::createImageView(uint32_t index) {
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

void vk_helper::createFramebuffer(uint32_t index) {
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

void vk_helper::recordCommandBuffer(uint32_t index) {
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

    VkDeviceSize offset = 0;
    mCmdBindVertexBuffers(mCommandBuffers[index], 0, 1, &mVertexBuffer, &offset);

    mCmdPushConstants(mCommandBuffers[index], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                      3 * sizeof(float), &fragData[0]);
    mCmdDraw(mCommandBuffers[index], 4, 1, 0, 0);

    mCmdPushConstants(mCommandBuffers[index], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                      3 * sizeof(float), &fragData[3]);
    mCmdDraw(mCommandBuffers[index], 4, 1, 2, 0);

    mCmdPushConstants(mCommandBuffers[index], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                      3 * sizeof(float), &fragData[6]);
    mCmdDraw(mCommandBuffers[index], 4, 1, 6, 0);

    mCmdPushConstants(mCommandBuffers[index], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                      3 * sizeof(float), &fragData[9]);
    mCmdDraw(mCommandBuffers[index], 4, 1, 8, 0);

    mCmdEndRenderPass(mCommandBuffers[index]);

    ASSERT(mEndCommandBuffer(mCommandBuffers[index]) == VK_SUCCESS);
}

void vk_helper::drawFrame() {
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

void vk_helper::destroy() {
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
