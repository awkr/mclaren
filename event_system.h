#pragma once

#include <cstdint>
#include <vector>

#define MAX_EVENT_CODES 20

enum EventCode {
    EVENT_CODE_UNKNOWN,
    EVENT_CODE_KEY_DOWN,
    EVENT_CODE_KEY_UP,
    EVENT_CODE_MOUSE_BUTTON_DOWN,
    EVENT_CODE_MOUSE_BUTTON_UP,
    EVENT_CODE_MOUSE_MOVE,
    EVENT_CODE_MOUSE_WHEEL,
    EVENT_CODE_MOUSE_DRAG_BEGIN,
    EVENT_CODE_MOUSE_DRAG,
    EVENT_CODE_MOUSE_DRAG_END,
};

struct EventContext {
    union { // 128 bytes
        int64_t i64[2];
        uint64_t u64[2];
        double f64[2];

        int32_t i32[4];
        uint32_t u32[4];
        float f32[4];

        int16_t i16[8];
        uint16_t u16[8];
        int8_t i8[16];
        uint8_t u8[16];

        struct {
            const void *ptr; // a pointer to a memory block of data, should be freed by sender or listener
            uint32_t size;   // the size of the data pointed to by ptr
        } raw_data;

        const char *str; // a free-form string, should be freed by sender or listener
    } data;
};

typedef bool (*PFN_on_event)(EventCode code, void *sender, void *listener, EventContext context);

struct EventRegistrant {
    void *listener;
    PFN_on_event fn;
};

struct EventRegistry {
    std::vector<EventRegistrant> registrants;
};

struct EventSystemState {
    EventRegistry event_registries[MAX_EVENT_CODES];
};

void event_system_create(EventSystemState **out_state);

void event_system_destroy(EventSystemState *state);

void event_system_update(EventSystemState *state);

void event_register(EventSystemState *state, EventCode code, void *listener, PFN_on_event fn);
bool event_unregister(EventSystemState *state, EventCode code, const void *listener, PFN_on_event fn);

/**
 * @brief Fire an event
 *
 * @param sender the object that is sending the event, can be NULL
 * @param context the event context
 * @returns true if the event was handled
 */
bool event_fire(EventSystemState *state, EventCode code, void *sender, EventContext context);
