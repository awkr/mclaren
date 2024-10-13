#include "input_system.h"
#include "event_system.h"
#include <cstring>

void input_system_create(struct EventSystemState *event_system_state, InputSystemState **out_state) {
    InputSystemState *state = new InputSystemState();
    state->event_system_state = event_system_state;
    *out_state = state;
}

void input_system_destroy(InputSystemState *state) {
    delete state;
}

void input_system_update(InputSystemState *state) {
    // copy current states to previous states
    memcpy(state->prev_key_states, state->key_states, sizeof(state->key_states));
    memcpy(state->prev_mouse_button_states, state->mouse_button_states, sizeof(state->mouse_button_states));
}

void input_process_key(InputSystemState *state, Key key, bool is_down) noexcept {
    if (state->key_states[key] == is_down) {
        return;
    }
    state->key_states[key] = is_down;

    EventContext context;
    context.data.u32[0] = key;
    event_fire(state->event_system_state, is_down ? EVENT_CODE_KEY_DOWN : EVENT_CODE_KEY_UP, state, context); // if the state changed, fire an event
}

void input_process_mouse_button(InputSystemState *state, MouseButton mouse_button, bool is_down, float x, float y) noexcept {
    state->mouse_button_states[mouse_button] = is_down;

    EventContext context;
    context.data.u32[0] = mouse_button;
    context.data.f32[1] = x;
    context.data.f32[2] = y;
    event_fire(state->event_system_state, is_down ? EVENT_CODE_MOUSE_BUTTON_DOWN : EVENT_CODE_MOUSE_BUTTON_UP, state, context);
}

void input_process_mouse_move(InputSystemState *state, float x, float y) noexcept {
    EventContext context;
    context.data.f32[0] = x;
    context.data.f32[1] = y;
    event_fire(state->event_system_state, EVENT_CODE_MOUSE_MOVE, state, context);
}

bool is_key_down(InputSystemState *state, Key key) noexcept {
    return state->key_states[key];
}

bool is_key_up(InputSystemState *state, Key key) noexcept {
    return !state->key_states[key];
}

bool was_key_down(InputSystemState *state, Key key) noexcept {
    return state->prev_key_states[key];
}

bool was_key_up(InputSystemState *state, Key key) noexcept {
    return !state->prev_key_states[key];
}
