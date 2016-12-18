#pragma once
#include "Compiler/ExprResult.h"
#include "Compiler/SrcPos.h"
#include "Compiler/NamedThread.h"
#include "Compiler/CodeGen.h"
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
		 * Constant (eg, number, string...).
		 */
		class Constant : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Constant(SrcPos pos, Int i);
			STORM_CTOR Constant(SrcPos pos, Long i);
			STORM_CTOR Constant(SrcPos pos, Float f);
			STORM_CTOR Constant(SrcPos pos, Str *str);
			STORM_CTOR Constant(SrcPos pos, Bool b);

			// Types
			enum CType {
				tInt,
				tFloat,
				tStr,
				tBool,
			};

			// Actual type.
			CType cType;

			// Value (if integer).
			Long intValue;

			// Value (if float).
			Float floatValue;

			// Value (if string).
			Str *strValue;

			// Value (if bool).
			Bool boolValue;

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Castable to?
			virtual Int STORM_FN castPenalty(Value to);

			// To string.
			virtual void output(wostream &out) const;

		protected:
			// Code for a string label.
			void strCode(CodeGen *state, CodeResult *r);

			// Code for an int label.
			void intCode(CodeGen *state, CodeResult *r);

			// Code for a float label.
			void floatCode(CodeGen *state, CodeResult *r);

			// Code for a bool label.
			void boolCode(CodeGen *state, CodeResult *r);
		};

		Constant *STORM_FN intConstant(SrcPos pos, Str *str);
		Constant *STORM_FN strConstant(SrcPos pos, Str *str);
		Constant *STORM_FN rawStrConstant(SrcPos pos, Str *str);
		Constant *STORM_FN floatConstant(SrcPos pos, Str *str);
		Constant *STORM_FN trueConstant(EnginePtr e, SrcPos pos);
		Constant *STORM_FN falseConstant(EnginePtr e, SrcPos pos);

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
