#pragma once
#include "Compiler/ExprResult.h"
#include "Compiler/SrcPos.h"
#include "Compiler/NamedThread.h"
#include "Compiler/CodeGen.h"
#include "Compiler/Syntax/SStr.h"
#include "Core/EnginePtr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Base class for expressions (no difference between statements and expressions in this language!).
		 */
		class Expr : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Expr(SrcPos pos);

			// Where is this expression located in the source code?
			SrcPos pos;

			// Result of an expression. Default is void.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Is it possible to cast this one expression to 'to'? < 0, no cast possible.
			virtual Int STORM_FN castPenalty(Value to);
		};


		/**
		 * Numeric literal.
		 */
		class NumLiteral : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR NumLiteral(SrcPos pos, Int i);
			STORM_CTOR NumLiteral(SrcPos pos, Long i);
			STORM_CTOR NumLiteral(SrcPos pos, Double f);

			// Specify type using a suffix. 'suffix' is one of the characters 'binlwfd', which each
			// corresponds to the first letter in one of the primitive types.
			void STORM_FN setType(Str *suffix);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Castable to?
			virtual Int STORM_FN castPenalty(Value to);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value (if integer).
			Long intValue;

			// Value (if float).
			Double floatValue;

			// Integer value?
			Bool isInt;

			// Specified type (if any).
			MAYBE(Type *) type;

			// Code for an int label.
			void intCode(CodeGen *state, CodeResult *r);

			// Code for a float label.
			void floatCode(CodeGen *state, CodeResult *r);
		};

		NumLiteral *STORM_FN intConstant(SrcPos pos, Str *str);
		NumLiteral *STORM_FN floatConstant(SrcPos pos, Str *str);

		/**
		 * String literal.
		 */
		class StrLiteral : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR StrLiteral(SrcPos pos, Str *str);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value.
			Str *value;
		};

		StrLiteral *STORM_FN strConstant(syntax::SStr *str);
		StrLiteral *STORM_FN strConstant(SrcPos pos, Str *str);
		StrLiteral *STORM_FN rawStrConstant(SrcPos pos, Str *str);

		/**
		 * Boolean literal.
		 */
		class BoolLiteral : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BoolLiteral(SrcPos pos, Bool value);

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Value.
			Bool value;
		};


		/**
		 * Dummy expression, tells that we're returning a value of a specific type, but will not
		 * generate any code.
		 */
		class DummyExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR DummyExpr(SrcPos pos, Value type);

			virtual ExprResult STORM_FN result();
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		private:
			Value type;
		};

	}
}
