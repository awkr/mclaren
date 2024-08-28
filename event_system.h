#pragma once

struct EventSystemState {
};

void event_system_create(EventSystemState **out_state);

void event_system_destroy(EventSystemState *state);

void event_system_update(EventSystemState *state);
