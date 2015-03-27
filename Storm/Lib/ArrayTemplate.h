#pragma once
#include "Array.h"
#include "Template.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * The array template class used in Storm.
	 */
	Template *arrayTemplate(Engine &e);

	/**
	 * The array type.
	 */
	class ArrayType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		ArrayType(const Value &param);

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
	};

	// See Array.h for 'arrayType' function.

}
