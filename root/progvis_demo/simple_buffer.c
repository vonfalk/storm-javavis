#include "wrap/thread.h"
#include "wrap/synch.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// The data structure.
struct buffer {
    // The single value inside the buffer.
    int value;
};

// Create a buffer.
struct buffer *buffer_create(void) {
    // Allocate memory and initialize it.
    struct buffer *buffer = malloc(sizeof(struct buffer));
    buffer->value = 0;
    return buffer;
}

// Deallocate a buffer.
void buffer_destroy(struct buffer *buffer) {
    free(buffer);
}

// Add an element to the buffer. If the buffer is full, the implementation shall
// wait for an element to be removed before inserting a new one.
void buffer_put(struct buffer *buffer, int value) {
    buffer->value = value;
}

// Remove an element from the buffer. If the buffer is empty, the implementation
// shall wait for an element to be inserted before removing it.
int buffer_get(struct buffer *buffer) {
    return buffer->value;
}

/**
 * Main-program. If the implementation above is correct, you should not have to
 * change anything here. You may want to modify "main" to test your
 * implementation though.
 */

// Thread producing data to the main thread.
void thread(struct buffer *buffer) {
    for (int i = 0; i < 10; i++) {
        buffer_put(buffer, i);
    }
}

int main(void) {
    struct buffer *buffer = buffer_create();

    // Skapa en tråd som producerar data till oss.
    thread_new(&thread, buffer);

    // Hämta data.
    for (int i = 0; i < 10; i++) {
        printf("Got: %d\n", buffer_get(buffer));
    }

    // Städa.
    buffer_destroy(buffer);

    return 0;
}
