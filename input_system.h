#pragma once

#define MAX_KEYS 256 // 最大支持 256 个按键

enum Key {
    KEY_UNKNOWN = 0,
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_Q,
    KEY_E,
    KEY_ESC,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SPACE,
};

struct InputSystemState {
    bool key_states[MAX_KEYS];
    bool prev_key_states[MAX_KEYS];
};

void input_system_create(InputSystemState **out_state);

void input_system_destroy(InputSystemState *state);

void input_system_update(InputSystemState *state);

void input_process_key(InputSystemState *state, Key key, bool is_down);

bool is_key_down(InputSystemState *state, Key key);
bool is_key_up(InputSystemState *state, Key key);

bool was_key_down(InputSystemState *state, Key key);
bool was_key_up(InputSystemState *state, Key key);
