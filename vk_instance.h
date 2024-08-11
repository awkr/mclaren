#pragma once

struct VkContext;

bool vk_create_instance(VkContext *vk_context, const char *app_name, bool is_debugging);

void vk_destroy_instance(VkContext *vk_context);
