#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "CodeGen.h"
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

			// Result of an expression. Default is null.
			virtual Value STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

			// Is it possible to cast this one expression to 'to'?
			virtual Bool STORM_FN castable(Value to);
		};


		/**
		 * Constant (eg, number, string...).
		 */
		class Constant : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Constant(Int i);
			Constant(int64 i);
			STORM_CTOR Constant(Float f);
			Constant(double d);
			STORM_CTOR Constant(Str *str);
			STORM_CTOR Constant(Bool b);

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
			virtual Value STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

			// Castable to?
			virtual Bool STORM_FN castable(Value to);

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

		Constant *STORM_FN intConstant(Par<SStr> str);

		Constant *STORM_FN strConstant(Par<SStr> str);

		Constant *STORM_FN floatConstant(Par<SStr> str);

		/**
		 * Dummy expression, tells that we're returning a value of a specific type, but will not
		 * generate any code.
		 */
		class DummyExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR DummyExpr(Value type);

			virtual Value STORM_FN result();
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

		private:
			Value type;
		};

	}
}
