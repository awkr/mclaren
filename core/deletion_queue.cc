#include "core/deletion_queue.h"

void DeletionQueue::push_function(void (*function)()) {
    deletions.push_back(function);
}

void DeletionQueue::flush() {
    while (!deletions.empty()) {
        deletions.front()();
        deletions.pop_front();
    }
}
