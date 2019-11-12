#include "VkHelper.h"

#define GET_PROC(F) F = reinterpret_cast<PFN_vk##F>(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vk" #F))
#define GET_INST_PROC(F) F = reinterpret_cast<PFN_vk##F>(vkGetInstanceProcAddr(instance, "vk" #F))
#define GET_DEV_PROC(F) F = reinterpret_cast<PFN_vk##F>(GetDeviceProcAddr(device, "vk" #F))

void VkHelper::initializeGlobalApis() {
    GET_PROC(CreateInstance);
    GET_PROC(EnumerateInstanceExtensionProperties);
    GET_PROC(EnumerateInstanceVersion);
}

void VkHelper::initializeInstanceApis(VkInstance instance) {
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
}

void VkHelper::initializeDeviceApis(VkDevice device) {
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
}
