#pragma once
#include "Engine.h"

/**
 * Create the data needed by this shared library. The 'Shared' module provides a default
 * implementation that simply returns null. Implement this function somewhere in your library
 * and it should override this one.
 *
 * This has to be in its own translation unit for the linker-time override to work on Windows.
 */
void *createLibData(storm::Engine &e);
void destroyLibData(void *data);
