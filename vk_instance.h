#pragma once

#include <cstdint>

struct VkContext;

bool vk_create_instance(VkContext *vk_context, const char *app_name, uint32_t api_version, bool is_debugging);

void vk_destroy_instance(VkContext *vk_context);
