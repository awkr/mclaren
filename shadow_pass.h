#pragma once

#include <volk.h>

struct ShadowPass {
  uint32_t shadow_map_size;
};

void create_shadow_pass(uint32_t shadow_map_size, ShadowPass **out_shadow_pass);
void destroy_shadow_pass(ShadowPass *shadow_pass);
void prepare_shadow_pass();
void cleanup_shadow_pass();
void resize_shadow_pass();
void update_shadow_pass();
void render_shadow_pass();
