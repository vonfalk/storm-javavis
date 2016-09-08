#pragma once
#include "../Listing.h"
#include "../Output.h"

namespace code {
	namespace x86 {

		// Output an entire Listing to an Output object.
		void output(Listing *src, Output *to);

	}
}
