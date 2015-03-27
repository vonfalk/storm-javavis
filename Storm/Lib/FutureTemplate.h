#pragma once
#include "Future.h"
#include "Template.h"
#include "Type.h"
#include "Code/Listing.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * The future template class used in Storm.
	 */
	Template *futureTemplate(Engine &e);

	/**
	 * The future type.
	 */
	class FutureType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		FutureType(const Value &param);

		// Parameter type.
		const Value param;

	protected:
		// Lazy loading.
		virtual void lazyLoad();

	private:
		// Load functions assuming param is an object.
		void loadClassFns();

		// Load functions assuming param is a value.
		void loadValueFns();

		// Generate code for post and result functions.
		code::Listing resultValue();
	};

	// See Future.h for 'futureType' function.

}
