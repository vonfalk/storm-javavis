#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "CodeGen.h"
#include "ExprResult.h"
#include "Code/Listing.h"
#include "Code/Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Base class for expressions. (no difference between statements and expressions here!)
		 */
		class Expr : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR Expr();

			// Result of an expression. Default is void.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

			// Is it possible to cast this one expression to 'to'? < 0, no cast possible.
			virtual Int STORM_FN castPenalty(Value to);
		};


		/**
		 * Constant (eg, number, string...).
		 */
		class Constant : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Constant(Int i);
			STORM_CTOR Constant(Long i);
			STORM_CTOR Constant(Float f);
			Constant(double d);
			STORM_CTOR Constant(Str *str);
			STORM_CTOR Constant(Bool b);
			Constant(const String &str);

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
			int64 intValue;

			// Value (if float).
			double floatValue;

			// Value (if string).
			String strValue;

			// Value (if bool).
			Bool boolValue;

			// Return value.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

			// Castable to?
			virtual Int STORM_FN castPenalty(Value to);

		protected:
			virtual void output(wostream &out) const;

			// Code for a string label.
			void strCode(Par<CodeGen> state, Par<CodeResult> r);

			// Code for an int label.
			void intCode(Par<CodeGen> state, Par<CodeResult> r);

			// Code for a float label.
			void floatCode(Par<CodeGen> state, Par<CodeResult> r);

			// Code for a bool label.
			void boolCode(Par<CodeGen> state, Par<CodeResult> r);

			// Generate string data.
			void strData(code::Listing &to);
		};

		// TODO: Take care of the 'pos' parameter in these functions!

		Constant *STORM_FN intConstant(SrcPos pos, Par<Str> str);

		Constant *STORM_FN strConstant(SrcPos pos, Par<Str> str);
		Constant *STORM_FN rawStrConstant(SrcPos pos, Par<Str> str);

		Constant *STORM_FN floatConstant(SrcPos pos, Par<Str> str);

		Constant *STORM_ENGINE_FN trueConstant(EnginePtr e, SrcPos pos);
		Constant *STORM_ENGINE_FN falseConstant(EnginePtr e, SrcPos pos);

		/**
		 * Dummy expression, tells that we're returning a value of a specific type, but will not
		 * generate any code.
		 */
		class DummyExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR DummyExpr(Value type);

			virtual ExprResult STORM_FN result();
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

		private:
			Value type;
		};

	}
}
