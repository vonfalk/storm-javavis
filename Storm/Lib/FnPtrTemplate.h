#pragma once
#include "FnPtr.h"
#include "Template.h"
#include "Type.h"
#include "CodeGenFwd.h"

namespace storm {
	STORM_PKG(core);

	// Add the FnPtr template class to the package given.
	void addFnPtrTemplate(Par<Package> to);

	/**
	 * The FnPtr type.
	 */
	class FnPtrType : public Type {
		STORM_CLASS;
	public:
		// Create. (first parameter is return value).
		FnPtrType(const vector<Value> &params);

		// Lazy loading.
		virtual bool loadAll();

	private:
		// Generate code for function calls.
		CodeGen *CODECALL callCode();
	};

	// See FnPtr.h for 'fnPtrType' function.
}
