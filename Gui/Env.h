#pragma once

/**
 * A list of environment variables used by the Gui library, so that it is easy to find all uses of them.
 */

// Environment variable to set render backend on Linux.
#define ENV_RENDER_BACKEND "STORM_RENDER_BACKEND"

// Environment variable to determine if separate X windows shall be allocated for windows that render things.
#define ENV_RENDER_X_WINDOW "STORM_RENDER_X_WINDOW"

// Environment variable to determine which workarounds to apply. Comma-separated.
#define ENV_RENDER_WORKAROUND "STORM_RENDER_WORKAROUND"
