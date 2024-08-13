#include "vk_device.h"
#include "vk_context.h"
#include "logging.h"

static bool
is_physical_device_present_supported(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    VkBool32 present_support = false;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if (present_support) { return true; }
    }
    return false;
}

static bool select_physical_device(VkContext *vk_context) {
    uint32_t physical_device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(vk_context->instance, &physical_device_count, nullptr);
    if (result != VK_SUCCESS || physical_device_count < 1) { return false; }

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    result = vkEnumeratePhysicalDevices(vk_context->instance, &physical_device_count, physical_devices.data());
    if (result != VK_SUCCESS) { return false; }

    VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties device_properties;
    for (VkPhysicalDevice physical_device: physical_devices) {
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);
        if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { // prefer discrete GPU
            if (is_physical_device_present_supported(physical_device, vk_context->surface)) {
                selected_physical_device = physical_device;
                break;
            }
        }
    }
    if (!selected_physical_device) { selected_physical_device = physical_devices[0]; } // just select the first one

    vkGetPhysicalDeviceProperties(selected_physical_device, &device_properties);
    log_info("vk physical device selected: %s, Vulkan %d.%d.%d", device_properties.deviceName,
             VK_VERSION_MAJOR(device_properties.apiVersion),
             VK_VERSION_MINOR(device_properties.apiVersion),
             VK_VERSION_PATCH(device_properties.apiVersion));

    vk_context->physical_device = selected_physical_device;
    return true;
}

bool get_queue_family_index(const std::vector<VkQueueFamilyProperties> &queue_families, VkQueueFlags flags,
                            uint32_t *queue_family_index) {
    for (uint32_t i = 0; i < queue_families.size(); ++i) {
        if ((queue_families[i].queueFlags & flags) == flags) {
            *queue_family_index = i;
            return true;
        }
    }
    return false;
}

bool vk_create_device(VkContext *vk_context) {
    if (!select_physical_device(vk_context)) { return false; }

    // set and check required device extensions
    std::vector<const char *> required_extensions;
    required_extensions.push_back("VK_KHR_swapchain");
    required_extensions.push_back("VK_KHR_portability_subset");
    required_extensions.push_back("VK_KHR_dynamic_rendering");
    required_extensions.push_back("VK_KHR_synchronization2");

    uint32_t extension_count = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(vk_context->physical_device, nullptr, &extension_count,
                                                           nullptr);
    if (result != VK_SUCCESS || extension_count < 1) { return false; }

    std::vector<VkExtensionProperties> extensions(extension_count);
    result = vkEnumerateDeviceExtensionProperties(vk_context->physical_device, nullptr, &extension_count,
                                                  extensions.data());
    if (result != VK_SUCCESS) { return false; }

    // for (const VkExtensionProperties &extension: extensions) {
    //     log_info("device extension: %s", extension.extensionName);
    // }

    bool found = false;
    for (const char *required_extension: required_extensions) {
        for (const VkExtensionProperties &extension: extensions) {
            if (strcmp(required_extension, extension.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) { return false; }
    }

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_context->physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_context->physical_device, &queue_family_count,
                                             queue_families.data());
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_count);
    std::vector<std::vector<float>> queue_priorities(queue_family_count);
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        queue_priorities[i].resize(queue_families[i].queueCount, 1.0f);

        VkDeviceQueueCreateInfo *queue_create_info = &queue_create_infos[i];
        queue_create_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info->queueFamilyIndex = i;
        queue_create_info->queueCount = queue_families[i].queueCount;
        queue_create_info->pQueuePriorities = queue_priorities[i].data();
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(vk_context->physical_device, &features);

    VkPhysicalDeviceFeatures required_device_features{};
    required_device_features.samplerAnisotropy = features.samplerAnisotropy;

    // VkPhysicalDeviceVulkan13Features physical_device_vulkan_13_features{};
    // physical_device_vulkan_13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    // physical_device_vulkan_13_features.synchronization2 = VK_TRUE;
    // physical_device_vulkan_13_features.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2_features{};
    synchronization2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2_features.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan_12_features{};
    vulkan_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan_12_features.timelineSemaphore = VK_TRUE;
    vulkan_12_features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
    vulkan_12_features.pNext = &synchronization2_features;

    VkDeviceCreateInfo device_create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = queue_create_infos.size();
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = required_extensions.size();
    device_create_info.ppEnabledExtensionNames = required_extensions.data();
    device_create_info.pEnabledFeatures = &required_device_features;
    device_create_info.pNext = &vulkan_12_features;
    result = vkCreateDevice(vk_context->physical_device, &device_create_info, nullptr, &vk_context->device);
    if (result != VK_SUCCESS) { return false; }

    volkLoadDevice(vk_context->device);

    // get graphics queue
    if (found = get_queue_family_index(queue_families, VK_QUEUE_GRAPHICS_BIT,
                                       &vk_context->graphics_queue_family_index); !found) { return false; }
    vkGetDeviceQueue(vk_context->device, vk_context->graphics_queue_family_index, 0, &vk_context->graphics_queue);

    return true;
}

void vk_destroy_device(VkContext *vk_context) { vkDestroyDevice(vk_context->device, nullptr); }
