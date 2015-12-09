#pragma once
#include "Value.h"
#include "Shared/CloneEnv.h"

namespace storm {

	/**
	 * Result from an expression. A result either returns a Value (which may be void), or a special
	 * value indicating that the expression will never return for some reason. For example, if a
	 * return statement has been found in a block (on all paths), that block will return this 'no
	 * return' value. The same holds true for paths always throwing an exception.
	 */
	class ExprResult {
		STORM_VALUE;
	public:
		// Return void.
		STORM_CTOR ExprResult();

		// Return a value.
		STORM_CTOR ExprResult(Value value);

		// Result type. If equal to 'noReturn', we never return.
		Value STORM_FN result() const;

		// Any result?
		Bool STORM_FN any() const;

		// Same type?
		Bool STORM_FN operator ==(const ExprResult &o) const;
		Bool STORM_FN operator !=(const ExprResult &o) const;

		// Deep copy.
		void STORM_FN deepCopy(Par<CloneEnv> env);

	private:
		// Return type.
		Value type;

		// Any result?
		bool returns;

		// Create 'no-return' value.
		ExprResult(bool any);

		// Allow 'noReturn' to create instances.
		friend ExprResult noReturn();
	};

	// Output.
	wostream &operator <<(wostream &to, const ExprResult &r);

	// To string.
	Str *STORM_ENGINE_FN toS(EnginePtr e, ExprResult from);

	// Get the 'no return' value.
	ExprResult STORM_FN noReturn();

}
