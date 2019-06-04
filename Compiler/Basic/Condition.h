#pragma once
#include "Expr.h"
#include "WeakCast.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A condition that is either a regular condition or a weak condition.
		 *
		 * Used to implement the logic common between the if- and the loop-statements.
		 */
		class Condition : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Condition();

			// Get a suitable position for this condition.
			virtual SrcPos pos();

			// Get the variable that will be created by this condition if it is successful.
			virtual MAYBE(LocalVar *) result();

			// Generate code for the condition. This will create and initialize the "result"
			// variable, if applicable. Will return a boolean value as the CodeResult value.
			virtual void code(CodeGen *state, CodeResult *ok);
		};


		/**
		 * A regular boolean condition.
		 */
		class BoolCondition : public Condition {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BoolCondition(Expr *expr);

			// Get a suitable position for this condition.
			virtual SrcPos pos();

			// Result variable.
			virtual MAYBE(LocalVar *) result();

			// Generate code.
			virtual void code(CodeGen *state, CodeResult *ok);

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// The expression we're evaluating.
			Expr *expr;
		};


		/**
		 * A weak cast used as a conditional.
		 */
		class WeakCondition : public Condition {
			STORM_CLASS;
		public:
			// Create with a weak cast created previously.
			STORM_CTOR WeakCondition(WeakCast *cast);

			// Create with a weak cast created previously and a suggested variable name.
			STORM_CTOR WeakCondition(syntax::SStr *varName, WeakCast *cast);

			// Get a suitable position for this condition.
			virtual SrcPos pos();

			// Result variable.
			virtual MAYBE(LocalVar *) result();

			// Generate code.
			virtual void code(CodeGen *state, CodeResult *ok);

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Weak cast used.
			WeakCast *cast;

			// Name of the additional variable, if any.
			syntax::SStr *varName;

			// Created variable, if any.
			MAYBE(LocalVar *) created;
		};


		// Create a suitable condition for an expression.
		// TODO: Make this extensible somehow!
		Condition *STORM_FN createCondition(Expr *expr);



		/**
		 * A block used to encapsulate the additional variable created by a successful weak cast.
		 */
		class CondSuccess : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR CondSuccess(SrcPos pos, Block *parent, Condition *cond);

			// Set the single contained expression. Throws if used more than once.
			void STORM_FN set(Expr *expr);

			// Set or replace the contained expression.
			void STORM_FN replace(Expr *expr);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code generation.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to);

			// Auto casting.
			virtual Int STORM_FN castPenalty(Value to);

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Contained expression.
			MAYBE(Expr *) expr;
		};


	}
}
