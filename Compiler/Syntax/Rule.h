#pragma once
#include "Decl.h"
#include "Compiler/Type.h"
#include "Compiler/Scope.h"
#include "Compiler/Basic/Param.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * A syntax rule.
		 *
		 * A rule is represented in the type system as a type. All productions specifying this rule
		 * inherit from this type, providing access to whatever was matched. The base class provides
		 * a member 'transform' for transforming the syntax tree as specified in the syntax file.
		 */
		class Rule : public Type {
			STORM_CLASS;
		public:
			// Create the rule.
			STORM_CTOR Rule(RuleDecl *rule, Scope scope);

			// Declared at.
			SrcPos pos;

			// Scope.
			Scope scope;

			// Get parameters and result.
			Array<bs::ValParam> *STORM_FN params();
			Value STORM_FN result();

		protected:
			// Lazy-loading.
			virtual Bool STORM_FN loadAll();

		private:
			// Rule declaration. Set to 'null' when it has been loaded.
			RuleDecl *decl;

			// Parameters to the transform function.
			Array<bs::ValParam> *tfmParams;

			// Result from the transform function.
			Value tfmResult;

			// Initialize parameters and result.
			void initTypes();

			// Generate the transform function.
			CodeGen *CODECALL createTransform();
		};

	}
}
