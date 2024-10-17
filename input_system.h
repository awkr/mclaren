#pragma once

#define MAX_KEYS 128 // 最大支持 128 个按键
#define MAX_BUTTONS 128 // 最大支持 128 个按钮

#include <cstdint>

enum Key {
    KEY_UNKNOWN = 0,
    KEY_A,
    KEY_D,
    KEY_E,
    KEY_Q,
    KEY_S,
    KEY_W,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ESC,
    KEY_SPACE,
};

enum MouseButton {
    MOUSE_BUTTON_UNKNOWN = 0,
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT,
};

struct InputSystemState {
    bool key_states[MAX_KEYS];
    bool prev_key_states[MAX_KEYS];
    bool mouse_button_states[MAX_BUTTONS];
    bool prev_mouse_button_states[MAX_BUTTONS];

    struct EventSystemState *event_system_state;
};

void input_system_create(struct EventSystemState *event_system_state, InputSystemState **out_state);

void input_system_destroy(InputSystemState *state);

void input_system_update(InputSystemState *state);

void input_process_key(InputSystemState *state, Key key, bool is_down) noexcept;
void input_process_mouse_button(InputSystemState *state, MouseButton mouse_button, bool is_down, float x, float y) noexcept;
void input_process_mouse_move(InputSystemState *state, float x, float y) noexcept;

bool is_key_down(InputSystemState *state, Key key) noexcept;
bool is_key_up(InputSystemState *state, Key key) noexcept;

bool was_key_down(InputSystemState *state, Key key) noexcept;
bool was_key_up(InputSystemState *state, Key key) noexcept;
