#pragma once

#include "camera.h"
#include "mesh_loader.h"
#include <cstdint>
#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;
struct SDL_Window;
struct ImGuiContext;

struct App {
    SDL_Window *window;
    VkContext *vk_context;
    uint64_t frame_number;
    uint32_t frame_index;
    VkImage drawable_image;
    VmaAllocation drawable_image_allocation;
    VkImageView drawable_image_view;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_set;

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;

    VkPipelineLayout mesh_pipeline_layout;
    VkPipeline mesh_pipeline;

    Geometry geometry;
    Camera camera;

    ImGuiContext *gui_context;
};

void app_create(SDL_Window *window, App **app);

void app_destroy(App *app);

void app_update(App *app);

void app_key_up(App *app, uint32_t key);

void app_key_down(App *app, uint32_t key);
