#pragma once
#include "Shared/Future.h"
#include "Template.h"
#include "Type.h"
#include "Code/Listing.h"

namespace storm {
	STORM_PKG(core.lang);

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

		// Lazy loading.
		virtual bool loadAll();

	private:
		// Load functions assuming param is an object.
		void loadClassFns();

		// Load functions assuming param is a value.
		void loadValueFns();

		// Load functions for the void type.
		void loadVoidFns();

		// Generate code for post and result functions.
		code::Listing resultValue();
	};

	// See Future.h for 'futureType' function.

}
