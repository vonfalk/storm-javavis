#pragma once
#include "ValueArray.h"
#include "Type.h"
#include "Core/EnginePtr.h"
#include "Code/Listing.h"

namespace storm {
	STORM_PKG(core.lang);

	// Create types for unknown implementations.
	Type *createFuture(Str *name, ValueArray *params);

	/**
	 * Type for futures.
	 */
	class FutureType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR FutureType(Str *name, Type *contents);

		// Parameter.
		Value param() const;

	protected:
		// Load.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;

		// Load different varieties.
		void loadVoid();
		void loadClass();
		void loadValue();

		// Generate code for retrieving values.
		code::Listing *resultValue();
	};

	Bool STORM_FN isFuture(Value v);
	Value STORM_FN unwrapFuture(Value v);
	Value STORM_FN wrapFuture(EnginePtr e, Value v);


}
