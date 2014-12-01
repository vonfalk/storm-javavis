#pragma once
#include "Code/Listing.h"

namespace storm {

	/**
	 * Describes the state of code generation.
	 */
	struct GenState {
		code::Listing &to;
		code::Frame &frame;
		code::Block block;
	};

}
