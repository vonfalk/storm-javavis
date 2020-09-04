#pragma once

/**
 * Configuration of the rendering mode used with Gtk+.
 *
 * These constants are used to produce and interpret a bitmask representing the current mode.
 */

// Use multithreaded rendering?
#define GTK_RENDER_MT 0x1
#define GTK_RENDER_IS_MT(x) ((x) & 0x1)

// Use software rendering?
#define GTK_RENDER_SW 0x2
#define GTK_RENDER_IS_SW(x) ((x) & 0x2)
