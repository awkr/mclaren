#pragma once

struct VkContext;

bool vk_create_device(VkContext *vk_context);

void vk_destroy_device(VkContext *vk_context);
