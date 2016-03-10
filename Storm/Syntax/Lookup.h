#pragma once
#include "Scope.h"

namespace storm {
	namespace syntax {

		/**
		 * Custom lookup for the syntax language.
		 *
		 * Resolves SStr class properly even though core.lang is not included.
		 */
		class SyntaxLookup : public ScopeExtra {
			STORM_CLASS;
		public:
			STORM_CTOR SyntaxLookup();

			// Lookup stuff.
			virtual MAYBE(Named) *STORM_FN find(const Scope &in, Par<SimpleName> name);

		private:
			// The core.lang package.
			Package *lang;
		};

	}
}
