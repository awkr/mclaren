#include "event_system.h"

void event_system_create(EventSystemState **out_state) {
    *out_state = new EventSystemState();
}

void event_system_destroy(EventSystemState *state) {
    delete state;
}

void event_system_update(EventSystemState *state) {
    //
}

void event_register(EventSystemState *state, EventCode code, void *listener, PFN_on_event fn) {
    EventRegistry *registry = &state->event_registries[code];

    EventRegistrant registrant;
    registrant.listener = listener;
    registrant.fn = fn;

    registry->registrants.push_back(registrant);
}

bool event_unregister(EventSystemState *state, EventCode code, const void *listener, PFN_on_event fn) {
    EventRegistry *registry = &state->event_registries[code];

    for (auto it = registry->registrants.begin(); it != registry->registrants.end(); ++it) {
        if (it->listener == listener && it->fn == fn) {
            registry->registrants.erase(it);
            return true;
        }
    }
    return false;
}

bool event_fire(EventSystemState *state, EventCode code, void *sender, EventContext context) {
    EventRegistry *registry = &state->event_registries[code];
    for (const EventRegistrant &registrant : registry->registrants) {
        if (registrant.fn(code, sender, registrant.listener, context)) {
            return true;
        }
    }
    return false;
}
