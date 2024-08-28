#pragma once

struct InputSystemState {
};

void input_system_create(InputSystemState **out_state);

void input_system_destroy(InputSystemState *state);

void input_system_update(InputSystemState *state);
