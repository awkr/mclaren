#pragma once

#include "vk_defines.h"

void vk_create_sampler(VkDevice device, VkFilter mag_filter, VkFilter min_filter, VkSampler *sampler);

void vk_destroy_sampler(VkDevice device, VkSampler sampler);
