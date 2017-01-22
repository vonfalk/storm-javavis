#pragma once

namespace code {

	/**
	 * Memory utilities.
	 */

	// Is the address readable (a pointer-sized type)?
	bool readable(void *addr);

	// Print some flags about the memory at addr.
	void memFlags(void *addr);

}
