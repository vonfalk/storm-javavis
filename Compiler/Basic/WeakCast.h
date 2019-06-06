#pragma once
#include "Block.h"
#include "Condition.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A weak cast. This is a condition that implements a type-cast that may fail. This
		 * implementation means that the failure has to be handled appropriately.
		 *
		 * This is an abstract class that implements some of the common functionality for all weak
		 * casts in the system.
		 */
		class WeakCast : public Condition {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakCast();

			/**
			 * Interface from Condition.
			 */

			// Get a suitable position for the cast.
			virtual SrcPos STORM_FN pos();

			// Get the result variable created by this cast.
			virtual MAYBE(LocalVar *) STORM_FN result();

			// Code generation. Initializes the created variable if required.
			virtual void STORM_FN code(CodeGen *state, CodeResult *ok);

			/**
			 * New interface implemented by subclasses.
			 */

			// Get the name of the variable we want to save the result of the weak cast to by
			// default. May return null if there is no obvious location to store the result in.
			virtual MAYBE(Str *) STORM_FN overwrite();

			// Get the type of the variable we wish to create.
			virtual Value STORM_FN resultType();

			// Generate code for the type-cast.
			virtual void STORM_FN castCode(CodeGen *state, CodeResult *ok, MAYBE(LocalVar *) var);

			/**
			 * New functionality provided here.
			 */

			// Set the name of the variable to store the result into.
			void STORM_FN name(syntax::SStr *name);

		protected:
			// Helper for the toS implementation. Outputs the specified variable if applicable.
			void STORM_FN output(StrBuf *to) const;

			// Default implementation of 'overwrite'. Extracts the name from an expression if possible.
			MAYBE(Str *) STORM_FN defaultOverwrite(Expr *expr);

		private:
			// Name of an explicitly specified variable, if any.
			MAYBE(syntax::SStr *) varName;

			// Created variable, if any.
			MAYBE(LocalVar *) created;
		};


		/**
		 * Weak downcast using 'as'.
		 */
		class WeakDowncast : public WeakCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakDowncast(Block *block, Expr *expr, SrcName *type);

			// Get a suitable position for the cast.
			virtual SrcPos STORM_FN pos();

			// Get variable we can write to.
			virtual MAYBE(Str *) STORM_FN overwrite();

			// Get the result type.
			virtual Value STORM_FN resultType();

			// Generate code.
			virtual void STORM_FN castCode(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Expression to cast.
			Expr *expr;

			// Type to cast to.
			Value to;
		};


		/**
		 * Weak cast from Maybe<T> to T.
		 */
		class WeakMaybeCast : public WeakCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakMaybeCast(Expr *expr);

			// Get a suitable position for the cast.
			virtual SrcPos STORM_FN pos();

			// Get variable we can write to.
			virtual MAYBE(Str *) STORM_FN overwrite();

			// Get the result type.
			virtual Value STORM_FN resultType();

			// Generate code.
			virtual void STORM_FN castCode(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Expression to cast.
			Expr *expr;

			// Generate code for class types.
			void classCode(MaybeClassType *c, CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var);

			// Genereate code for value types.
			void valueCode(MaybeValueType *c, CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var);
		};


		// Figure out what kind of weak cast to create based on the type of the lhs of expressions like <lhs> as <type>.
		WeakCast *STORM_FN weakAsCast(Block *block, Expr *expr, SrcName *type);

	}
}
