#pragma once

struct VkContext;

void vk_create_allocator(VkContext *vk_context);

void vk_destroy_allocator(VkContext *vk_context);
