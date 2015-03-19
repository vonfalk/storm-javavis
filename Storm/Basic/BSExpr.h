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

			// Result of an expression. Default is null. TODO: STORM_FN
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &state, GenResult &r);
		};


		/**
		 * Constant (eg, number, string...).
		 */
		class Constant : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Constant(Int i);
			STORM_CTOR Constant(Par<Str> str);

			// Types
			enum Type {
				tInt,
				tStr,
			};

			// Actual type.
			Type cType;

			// Value (if integer).
			Int intValue;

			// Value (if string).
			Auto<Str> strValue;

			// Return value.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &state, GenResult &r);

		protected:
			virtual void output(wostream &out) const;

			// Code for a string label.
			void strCode(const GenState &state, GenResult &r);

			// Code for an int label.
			void intCode(const GenState &state, GenResult &r);

			// Generate string data.
			void strData(code::Listing &to);
		};

		Constant *STORM_FN intConstant(Par<SStr> str);

		Constant *STORM_FN strConstant(Par<SStr> str);

	}
}
