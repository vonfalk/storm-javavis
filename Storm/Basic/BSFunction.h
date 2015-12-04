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

		class BSFunction;

		/**
		 * Function declaration. This holds the needed information to create each function
		 * later on. It would be nice to make this one inherit from Function, but that
		 * can not be done until we have proper support for arrays.
		 */
		class FunctionDecl : public ObjectOn<Compiler> {
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
			STORM_VAR SrcPos pos;
			STORM_VAR Auto<SStr> name;
			STORM_VAR Auto<TypeName> result;
			STORM_VAR Auto<Params> params;
			STORM_VAR Auto<TypeName> thread; // may be null
			STORM_VAR Auto<SStr> contents;

			// Create a corresponding function.
			Function *STORM_FN asFunction(const Scope &scope);

			// Temporary solution for updating a function.
			void STORM_FN update(Par<BSFunction> fn, const Scope &scope);

			// Get our name as a NamePart.
			NamePart *STORM_FN namePart(const Scope &scope) const;
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

			// Temporary solution for updating a function.
			void update(const vector<String> &names, Par<SStr> contents, const SrcPos &pos);
			Bool STORM_FN update(Par<ArrayP<Str>> names, Par<SStr> contents);
			void STORM_FN update(Par<BSFunction> from);

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
			CodeGen *CODECALL generateCode();

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

			// Store the result type.
			STORM_VAR Value type;
		};

	}
}
