#include "vk_swapchain.h"
#include "logging.h"
#include "vk_context.h"
#include "vk_image_view.h"

static bool
select_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkSurfaceFormatKHR *surface_format) {
    uint32_t format_count;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    if (result != VK_SUCCESS || format_count < 1) { return false; }
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
    if (result != VK_SUCCESS || format_count < 1) { return false; }

    *surface_format = formats[0];
    for (const VkSurfaceFormatKHR &format: formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM) { // preferred format
            *surface_format = format;
            break;
        }
    }
    return true;
}

bool vk_create_swapchain(VkContext *vk_context, uint32_t width, uint32_t height) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_context->physical_device, vk_context->surface,
                                                                &surface_capabilities);
    if (result != VK_SUCCESS) { return false; }

    VkExtent2D surface_size;
    if (surface_capabilities.currentExtent.width == UINT32_MAX) {
        surface_size = {width, height};
    } else {
        surface_size = surface_capabilities.currentExtent;
    }

    VkSurfaceFormatKHR surface_format;
    if (!select_surface_format(vk_context->physical_device, vk_context->surface, &surface_format)) { return false; }


    uint32_t present_mode_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_context->physical_device, vk_context->surface,
                                                       &present_mode_count, nullptr);
    if (result != VK_SUCCESS) { return false; }

    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_context->physical_device, vk_context->surface,
                                                       &present_mode_count,
                                                       present_modes.data());
    if (result != VK_SUCCESS) { return false; }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &mode: present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    uint32_t desired_image_count = 3;
    if (desired_image_count < surface_capabilities.minImageCount) {
        desired_image_count = surface_capabilities.minImageCount;
    }
    if (surface_capabilities.maxImageCount > 0 && desired_image_count > surface_capabilities.maxImageCount) {
        desired_image_count = surface_capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform = surface_capabilities.currentTransform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
        composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
        composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    } else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
        composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = vk_context->surface;
    swapchain_info.minImageCount = desired_image_count;
    swapchain_info.imageFormat = surface_format.format;
    swapchain_info.imageColorSpace = surface_format.colorSpace;
    swapchain_info.imageExtent = surface_size;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = pre_transform;
    swapchain_info.compositeAlpha = composite;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(vk_context->device, &swapchain_info, nullptr, &vk_context->swapchain);
    if (result != VK_SUCCESS) { return false; }

    vk_context->swapchain_extent = surface_size;
    vk_context->swapchain_image_format = surface_format.format;
    vk_context->swapchain_image_count = desired_image_count;

    uint32_t image_count;
    result = vkGetSwapchainImagesKHR(vk_context->device, vk_context->swapchain, &image_count, nullptr);
    if (result != VK_SUCCESS) { return false; }

    vk_context->swapchain_images.resize(image_count);
    result = vkGetSwapchainImagesKHR(vk_context->device, vk_context->swapchain, &image_count,
                                     vk_context->swapchain_images.data());
    if (result != VK_SUCCESS) { return false; }

    for (size_t i = 0; i < desired_image_count; ++i) {
      VkImageView image_view = VK_NULL_HANDLE;
      vk_create_image_view(vk_context->device, vk_context->swapchain_images[i], surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, &image_view);
      vk_context->swapchain_image_views.push_back(image_view);
    }

    log_debug("vk swapchain created");

    return true;
}

void vk_destroy_swapchain(VkContext *vk_context) {
    for (uint16_t i = 0; i < vk_context->swapchain_image_count; ++i) {
        vk_destroy_image_view(vk_context->device, vk_context->swapchain_image_views[i]);
    }
    vkDestroySwapchainKHR(vk_context->device, vk_context->swapchain, nullptr);
    log_debug("vk swapchain destroyed");
}
