#include "input_system.h"
#include <cstring>

void input_system_create(InputSystemState **out_state) {
    *out_state = new InputSystemState();
}

void input_system_destroy(InputSystemState *state) {
    delete state;
}

void input_system_update(InputSystemState *state) {
    // copy current key states to previous key states
    memcpy(state->prev_key_states, state->key_states, sizeof(state->key_states));
}

void input_process_key(InputSystemState *state, Key key, bool is_down) {
    state->key_states[key] = is_down;
}

bool is_key_down(InputSystemState *state, Key key) {
    return state->key_states[key];
}

bool is_key_up(InputSystemState *state, Key key) {
    return !state->key_states[key];
}

bool was_key_down(InputSystemState *state, Key key) {
    return state->prev_key_states[key];
}

bool was_key_up(InputSystemState *state, Key key) {
    return !state->prev_key_states[key];
}
