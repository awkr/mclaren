#include "vk_instance.h"
#include "logging.h"
#include "vk_context.h"
#include <vector>

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                    void *user_data);

bool vk_create_instance(VkContext *vk_context, const char *app_name, uint32_t api_version) {
  std::vector<const char *> request_instance_layers;
  request_instance_layers.push_back("VK_LAYER_KHRONOS_validation");

  std::vector<const char *> request_instance_extensions;
  request_instance_extensions.push_back("VK_KHR_surface");
#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
  request_instance_extensions.push_back("VK_EXT_metal_surface");
#elif defined(PLATFORM_WINDOWS)
  request_instance_extensions.push_back("VK_KHR_win32_surface");
#elif defined(PLATFORM_ANDROID)
  request_instance_extensions.push_back("VK_KHR_android_surface");
#elif defined(PLATFORM_LINUX)
  request_instance_extensions.push_back("VK_KHR_xcb_surface");
#endif
  request_instance_extensions.push_back("VK_EXT_debug_utils");
  request_instance_extensions.push_back("VK_KHR_portability_enumeration");
  request_instance_extensions.push_back("VK_KHR_get_physical_device_properties2");

  {
    uint32_t available_instance_layer_count = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&available_instance_layer_count, nullptr);
    if (result != VK_SUCCESS) {
      return false;
    }
    std::vector<VkLayerProperties> available_instance_layers(available_instance_layer_count);
    result = vkEnumerateInstanceLayerProperties(&available_instance_layer_count, available_instance_layers.data());
    if (result != VK_SUCCESS) {
      return false;
    }

    bool found = false;
    for (const auto &request_layer : request_instance_layers) {
      for (const auto &layer : available_instance_layers) {
        if (strcmp(layer.layerName, request_layer) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
  }

  {
    uint32_t available_instance_extension_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &available_instance_extension_count, nullptr);
    if (result != VK_SUCCESS) {
      return false;
    }
    std::vector<VkExtensionProperties> available_instance_extensions(available_instance_extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &available_instance_extension_count, available_instance_extensions.data());
    if (result != VK_SUCCESS) {
      return false;
    }

    bool found = false;
    for (const auto &request_extension : request_instance_extensions) {
      for (const auto &extension : available_instance_extensions) {
        if (strcmp(extension.extensionName, request_extension) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
  }

  // create instance
  VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = app_name;
  app_info.pEngineName = "aki engine";
  app_info.apiVersion = api_version;

  VkInstanceCreateInfo instance_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledLayerCount = request_instance_layers.size();
  instance_create_info.ppEnabledLayerNames = request_instance_layers.data();
  instance_create_info.enabledExtensionCount = request_instance_extensions.size();
  instance_create_info.ppEnabledExtensionNames = request_instance_extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_utils_messenger_create_info.pfnUserCallback = debug_utils_callback;

  std::vector<VkValidationFeatureEnableEXT> enabled_validation_features;
  // enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
  enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
  enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
  enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
  enabled_validation_features.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);

  VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
  validation_features.enabledValidationFeatureCount = enabled_validation_features.size();
  validation_features.pEnabledValidationFeatures = enabled_validation_features.data();
  validation_features.pNext = &debug_utils_messenger_create_info;

  instance_create_info.pNext = &validation_features;

  VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vk_context->instance);
  if (result != VK_SUCCESS) {
    return false;
  }

  volkLoadInstance(vk_context->instance);

  result = vkCreateDebugUtilsMessengerEXT(vk_context->instance, &debug_utils_messenger_create_info, nullptr, &vk_context->debug_utils_messenger);
  if (result != VK_SUCCESS) {
    return false;
  }

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
    ASSERT(false);
    exit(1);
  }
  return VK_FALSE;
}
