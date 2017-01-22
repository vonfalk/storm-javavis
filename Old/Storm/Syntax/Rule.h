#pragma once
#include "Storm/Type.h"
#include "Parse.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * A syntax rule.
		 *
		 * A rule is represented in the type system as a type. All options specifying this rule
		 * inherit from this type, providing access to what was matched. The base class provides a
		 * member 'eval' for transforming the syntax tree as specified in the syntax file.
		 */
		class Rule : public Type {
			STORM_CLASS;
		public:
			// Create the rule.
			STORM_CTOR Rule(Par<RuleDecl> rule, Scope scope);

			// Declared at.
			STORM_VAR SrcPos pos;

			// Scope.
			Scope scope;

			// Get parameters and result.
			Array<Value> *STORM_FN params();
			ArrayP<Str> *STORM_FN names();
			Value STORM_FN result();

			// Lazy-loading.
			virtual Bool STORM_FN loadAll();

			// Proper unloading.
			virtual void clear();

		private:
			// The rule declaration. Set to null when it has been loaded.
			Auto<RuleDecl> decl;

			// Parameters to the transform function.
			Auto<Array<Value>> tfmParams;

			// Parameter names to the transform function.
			Auto<ArrayP<Str>> tfmNames;

			// Result from the transform function.
			Value tfmResult;

			// Remember our throwing function.
			Auto<Function> throwFn;

			// Initialize tfmParams and tfmResult.
			void initTypes();

			// Create the 'transform' function.
			CodeGen *CODECALL createTransform();
		};

	}
}
