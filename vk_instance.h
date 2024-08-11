#pragma once

struct VkContext;

bool vk_create_instance(VkContext *vk_context, const char *app_name, bool enable_debugging);

void vk_destroy_instance(VkContext *vk_context);
