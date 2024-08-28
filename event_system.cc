#include "event_system.h"

void event_system_create(EventSystemState **out_state) {
    *out_state = new EventSystemState();
}

void event_system_destroy(EventSystemState *state) {
    delete state;
}

void event_system_update(EventSystemState *state) {}
