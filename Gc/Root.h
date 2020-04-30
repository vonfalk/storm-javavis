#pragma once
#include "OS/InlineSet.h"

namespace storm {

	class Gc;

	/**
	 * Base class for roots.
	 *
	 * Contains basic tracking, so that a Gc implementation may keep track of all roots.
	 */
	class GcRoot : public os::SetMember<GcRoot> {
	public:
		virtual ~GcRoot() {}

		Gc *owner;
	};

}
