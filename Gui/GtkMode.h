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


// These three flags are mutually exclusive. Do not pick more than one!

// Just swap buffers whenever ready.
#define GTK_RENDER_SWAP 0x00
#define GTK_RENDER_IS_SWAP(x) (((x) & 0xF0) == 0x00)

// Synchronize the SwapBuffer call with Gtk+.
#define GTK_RENDER_SYNC 0x10
#define GTK_RENDER_IS_SYNC(x) (((x) & 0xF0) == 0x10)

// Copy the GL context to the target GL context when Gtk+ would paint.
#define GTK_RENDER_COPY 0x20
#define GTK_RENDER_IS_COPY(x) (((x) & 0xF0) == 0x20)

// Copy the GL context to cairo? This might be slow, but it is more reliable.
#define GTK_RENDER_CAIRO 0x40
#define GTK_RENDER_IS_CAIRO(x) (((x) & 0xF0) == 0x40)

// More ideas here...
