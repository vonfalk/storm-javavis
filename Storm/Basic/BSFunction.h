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
		class FnBody;

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
			STORM_VAR MAYBE(Auto<TypeName>) thread;
			STORM_VAR Auto<SStr> contents;

			// Create a corresponding function.
			Function *STORM_FN asFunction(const Scope &scope);

			// Temporary solution for updating a function.
			void STORM_FN update(Par<BSFunction> fn, const Scope &scope);

			// Get our name as a NamePart.
			NamePart *STORM_FN namePart(const Scope &scope) const;
		};

		/**
		 * Raw function that will not read syntax trees by parsing a string.
		 */
		class BSRawFn : public Function {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSRawFn(Value result, Par<SStr> name, Par<Params> params,
							Scope scope, MAYBE(Par<NamedThread>) thread);

			// Declared at.
			SrcPos pos;

			// Scope.
			const Scope scope;

			// Add function parameters to a block. Mainly for internal use.
			void STORM_FN addParams(Par<Block> block);

			// Override this to create the syntax tree to compile.
			virtual FnBody *STORM_FN createBody();

			// Re-compile at next execution.
			void STORM_FN reset();

		protected:
			// Parameter names.
			vector<String> paramNames;

			// Generate code.
			CodeGen *CODECALL generateCode();
		};


		/**
		 * Function in the BS language.
		 */
		class BSFunction : public BSRawFn {
			STORM_CLASS;
		public:
			// Create a function.
			STORM_CTOR BSFunction(Value result, Par<SStr> name, Par<Params> params, Scope scope,
								MAYBE(Par<NamedThread>) thread, Par<SStr> contents);

			// Temporary solution for updating a function.
			void update(const vector<String> &names, Par<SStr> contents, const SrcPos &pos);
			Bool STORM_FN update(Par<ArrayP<Str>> names, Par<SStr> contents);
			void STORM_FN update(Par<BSFunction> from);

			// Create the body from our string.
			virtual FnBody *STORM_FN createBody();

		private:
			// Code.
			Auto<SStr> contents;
		};


		/**
		 * Contents of a function.
		 */
		class FnBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR FnBody(Par<BSRawFn> owner);

			// Store the result type.
			STORM_VAR Value type;
		};

	}
}
