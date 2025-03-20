#include "vk_device.h"
#include "logging.h"
#include "vk_context.h"

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
    log_debug("physical device properties:\n"
              "  uniform buffer alignment: %d\n"
              "  storage buffer alignment: %d",
              device_properties.limits.minUniformBufferOffsetAlignment,
              device_properties.limits.minStorageBufferOffsetAlignment);

    // log_info("max descriptor set samplers: %d\n"
    //          "max descriptor set sampled images: %d\n"
    //          "max descriptor set storage buffers: %d\n"
    //          "max descriptor set storage images: %d",
    //          device_properties.limits.maxDescriptorSetSamplers,
    //          device_properties.limits.maxDescriptorSetSampledImages, // combined image samplers
    //          device_properties.limits.maxDescriptorSetStorageBuffers,
    //          device_properties.limits.maxDescriptorSetStorageImages);

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
    required_extensions.push_back("VK_KHR_synchronization2");
    required_extensions.push_back("VK_KHR_copy_commands2");
    required_extensions.push_back("VK_KHR_fragment_shader_barycentric");
    required_extensions.push_back("VK_EXT_extended_dynamic_state");
    required_extensions.push_back("VK_EXT_extended_dynamic_state2");
    required_extensions.push_back("VK_EXT_descriptor_indexing");

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

    // query physical device properties
    VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties{};
    descriptor_indexing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;

    VkPhysicalDeviceProperties2KHR device_properties_2{};
    device_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    device_properties_2.pNext = &descriptor_indexing_properties;
    vkGetPhysicalDeviceProperties2KHR(vk_context->physical_device, &device_properties_2);

    VkPhysicalDeviceFeatures physical_device_features{};
    vkGetPhysicalDeviceFeatures(vk_context->physical_device, &physical_device_features);

    VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures separate_depth_stencil_features{};
    separate_depth_stencil_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
    separate_depth_stencil_features.separateDepthStencilLayouts = VK_TRUE;

    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extended_dynamic_state2_features{};
    extended_dynamic_state2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
    extended_dynamic_state2_features.extendedDynamicState2 = VK_TRUE;
    extended_dynamic_state2_features.pNext = &separate_depth_stencil_features;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features{};
    extended_dynamic_state_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extended_dynamic_state_features.extendedDynamicState = VK_TRUE;
    extended_dynamic_state_features.pNext = &extended_dynamic_state2_features;

    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragment_shader_barycentric_features{};
    fragment_shader_barycentric_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
    fragment_shader_barycentric_features.fragmentShaderBarycentric = VK_TRUE;
    fragment_shader_barycentric_features.pNext = &extended_dynamic_state_features;

    VkPhysicalDeviceSynchronization2Features synchronization2_features{};
    synchronization2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2_features.synchronization2 = VK_TRUE;
    synchronization2_features.pNext = &fragment_shader_barycentric_features;

    VkPhysicalDeviceBufferDeviceAddressFeatures device_address_features{};
    device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    device_address_features.bufferDeviceAddress = VK_TRUE;
    device_address_features.pNext = &synchronization2_features;

    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features{};
    timeline_semaphore_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    timeline_semaphore_features.timelineSemaphore = VK_TRUE;
    timeline_semaphore_features.pNext = &device_address_features;

    VkPhysicalDevice8BitStorageFeatures storage_8_bit_features{};
    storage_8_bit_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    storage_8_bit_features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
    storage_8_bit_features.pNext = &timeline_semaphore_features;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_set_indexing_features{};
    descriptor_set_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_set_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_set_indexing_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptor_set_indexing_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptor_set_indexing_features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptor_set_indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    descriptor_set_indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_set_indexing_features.runtimeDescriptorArray = VK_TRUE;
    descriptor_set_indexing_features.pNext = &storage_8_bit_features;

    VkPhysicalDeviceFeatures2 device_features_2{};
    device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features_2.features.samplerAnisotropy = physical_device_features.samplerAnisotropy;
    device_features_2.features.fillModeNonSolid = physical_device_features.fillModeNonSolid;
    device_features_2.features.wideLines = physical_device_features.wideLines;
    device_features_2.features.depthBiasClamp = physical_device_features.depthBiasClamp;
    device_features_2.features.fragmentStoresAndAtomics = physical_device_features.fragmentStoresAndAtomics;
    device_features_2.pNext = &descriptor_set_indexing_features;

    VkDeviceCreateInfo device_create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = queue_create_infos.size();
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = required_extensions.size();
    device_create_info.ppEnabledExtensionNames = required_extensions.data();
    device_create_info.pEnabledFeatures = nullptr;
    device_create_info.pNext = &device_features_2;
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
