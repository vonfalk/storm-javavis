#pragma once
#include "Variable.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Layout of variables inside a type.
	 *
	 * Currently, allocates the types in the order they were added to the layout, but in the future
	 * it is probably desirable to order the members in increasing size or similar to compact the
	 * layout as much as possible while keeping alignment requirements.
	 */
	class Layout : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Layout();

		// Add a variable to the layout.
		void STORM_FN add(MemberVar *var);
		void STORM_FN add(Named *n);

		// Lay out all variables in memory. Returns the total size of the creation.
		Size STORM_FN doLayout(Size parentSize);

	private:
		// All members in here, in the order they are laid out.
		Array<MemberVar *> *vars;
	};

}
