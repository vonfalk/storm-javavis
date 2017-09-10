#pragma once
#include "../Listing.h"
#include "../Output.h"

namespace code {
	namespace x64 {

		// Output an entire Listing to an Output object.
		void output(Listing *src, Output *to);

	}
}
