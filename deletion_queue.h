#pragma once

#include <deque>

struct DeletionQueue {
    void push_function(void (*function)());

    void flush();

    std::deque<void (*)()> deletions;
};
