#include "input_system.h"

void input_system_create(InputSystemState **out_state) {
    *out_state = new InputSystemState();
}

void input_system_destroy(InputSystemState *state) {
    delete state;
}

void input_system_update(InputSystemState *state) {}
