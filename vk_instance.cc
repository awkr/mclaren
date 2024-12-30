#include "vk_instance.h"
#include "logging.h"
#include "vk_context.h"
#include <vector>

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                    void *user_data);

bool vk_create_instance(VkContext *vk_context, const char *app_name, uint32_t api_version, bool is_debugging) {
    // set and check required instance extensions
    std::vector<const char *> required_extensions;
    required_extensions.push_back("VK_KHR_surface");
#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
    required_extensions.push_back("VK_EXT_metal_surface");
#elif defined(PLATFORM_WINDOWS)
    required_extensions.push_back("VK_KHR_win32_surface");
#elif defined(PLATFORM_ANDROID)
    required_extensions.push_back("VK_KHR_android_surface");
#elif defined(PLATFORM_LINUX)
    required_extensions.push_back("VK_KHR_xcb_surface");
#endif
    if (is_debugging) { required_extensions.push_back("VK_EXT_debug_utils"); }
    required_extensions.push_back("VK_KHR_portability_enumeration");
    required_extensions.push_back("VK_KHR_get_physical_device_properties2");

    uint32_t extension_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (result != VK_SUCCESS) { return false; }

    std::vector<VkExtensionProperties> extensions(extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
    if (result != VK_SUCCESS) { return false; }

    bool found = false;
    for (const auto &required_extension: required_extensions) {
        for (const auto &extension: extensions) {
            if (strcmp(extension.extensionName, required_extension) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    // set and check required instance layers
    std::vector<const char *> required_layers;
    if (is_debugging) { required_layers.push_back("VK_LAYER_KHRONOS_validation"); }

    uint32_t layer_count = 0;
    result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    if (result != VK_SUCCESS) { return false; }

    std::vector<VkLayerProperties> layers(layer_count);
    result = vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
    if (result != VK_SUCCESS) { return false; }

    found = false;
    for (const auto &required_layer: required_layers) {
        for (const auto &layer: layers) {
            if (strcmp(layer.layerName, required_layer) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    // create instance
    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = app_name;
    app_info.pEngineName = "aki engine";
    app_info.apiVersion = api_version;

    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = required_layers.size();
    instance_info.ppEnabledLayerNames = required_layers.data();
    instance_info.enabledExtensionCount = required_extensions.size();
    instance_info.ppEnabledExtensionNames = required_extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    std::vector<VkValidationFeatureEnableEXT> enabled_validation_features;
    VkValidationFeaturesEXT validation_features_info{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    if (is_debugging) {
        debug_utils_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_utils_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_utils_info.pfnUserCallback = debug_utils_callback;
        instance_info.pNext = &debug_utils_info;

        enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
        enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
        enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
        enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);

        validation_features_info.enabledValidationFeatureCount = enabled_validation_features.size();
        validation_features_info.pEnabledValidationFeatures = enabled_validation_features.data();
        validation_features_info.pNext = instance_info.pNext;
        instance_info.pNext = &validation_features_info;
    }

    result = vkCreateInstance(&instance_info, nullptr, &vk_context->instance);
    if (result != VK_SUCCESS) { return false; }

    volkLoadInstance(vk_context->instance);

    if (is_debugging) {
        result = vkCreateDebugUtilsMessengerEXT(vk_context->instance, &debug_utils_info, nullptr,
                                                &vk_context->debug_utils_messenger);
        if (result != VK_SUCCESS) { return false; }
    }

    vk_context->api_version = api_version;
    vk_context->is_debugging_mode = is_debugging;

    return true;
}

void vk_destroy_instance(VkContext *vk_context) {
    if (vk_context->debug_utils_messenger) {
        vkDestroyDebugUtilsMessengerEXT(vk_context->instance, vk_context->debug_utils_messenger, nullptr);
    }
    vkDestroyInstance(vk_context->instance, nullptr);
    volkFinalize();
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                    void *user_data) {
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        log_debug("%s", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        log_info("%s", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        log_warning("%s", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        log_error("%s", callback_data->pMessage);
        // ASSERT(false);
      exit(1);
    }
    return VK_FALSE;
}
