#pragma once
#include "Std.h"
#include "Basic/BSParams.h"
#include "Function.h"

namespace storm {
	namespace bs {

		/**
		 * Function declaration. This holds the needed information to create each function
		 * later on. It would be nice to make this one inherit from Function, but that
		 * can not be done until we have proper support for arrays.
		 */
		class FunctionDecl : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR FunctionDecl(SrcPos pos,
									Auto<SStr> name,
									Auto<TypeName> result,
									Auto<Params> params,
									Auto<SStr> contents);

			// Values.
			SrcPos pos;
			Auto<SStr> name;
			Auto<TypeName> result;
			Auto<Params> params;
			Auto<SStr> contents;

			// Create a corresponding function.
			Function *asFunction(const Scope &scope);
		};


		/**
		 * Function in the BS language.
		 */
		class BSFunction : public Function {
			STORM_CLASS;
		public:
			BSFunction(Value result, const String &name, const vector<Value> &params, Auto<Str> contents);

			virtual void *pointer() const;
		private:
			// Code.
			Auto<Str> contents;
		};

	}
}
