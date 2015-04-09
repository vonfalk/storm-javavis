#pragma once
#include "Std.h"
#include "Basic/BSParams.h"
#include "Function.h"
#include "BSBlock.h"
#include "BSExpr.h"
#include "BSScope.h"
#include "BSVar.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Function declaration. This holds the needed information to create each function
		 * later on. It would be nice to make this one inherit from Function, but that
		 * can not be done until we have proper support for arrays.
		 */
		class FunctionDecl : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR FunctionDecl(SrcPos pos,
									Par<SStr> name,
									Par<TypeName> result,
									Par<Params> params,
									Par<SStr> contents);

			STORM_CTOR FunctionDecl(SrcPos pos,
									Par<SStr> name,
									Par<TypeName> result,
									Par<Params> params,
									Par<TypeName> thread,
									Par<SStr> contents);

			// Values.
			SrcPos pos;
			Auto<SStr> name;
			Auto<TypeName> result;
			Auto<Params> params;
			Auto<TypeName> thread; // may be null
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
			BSFunction(Value result, const String &name, const vector<Value> &params,
					const vector<String> &names, const Scope &scope, Par<SStr> contents,
					Par<NamedThread> thread, const SrcPos &pos, bool isMember);

			// Declared at.
			SrcPos pos;

			// Scope.
			const Scope scope;

			// Add function parameters to a block.
			void addParams(Par<Block> block);

		private:
			// Code.
			Auto<SStr> contents;

			// Parameter names.
			vector<String> paramNames;

			// Generate code.
			CodeGen *generateCode();

			// Is a member function (ie this as first param?)
			bool isMember;
		};


		/**
		 * Contents of a function.
		 */
		class FnBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR FnBody(Par<BSFunction> owner);
		};

	}
}
