#pragma once
#include "Array.h"
#include "Template.h"
#include "Type.h"

namespace storm {

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

	protected:
		// Lazy loading.
		virtual void lazyLoad();

	private:
		// Parameter type.
		Value param;

		// Load functions assuming param is an object.
		void loadClassFns();
	};
}
