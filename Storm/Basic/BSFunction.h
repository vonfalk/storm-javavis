#pragma once
#include "Std.h"
#include "Basic/BSParams.h"
#include "Function.h"
#include "SyntaxEnv.h"
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
		 * Function declaration. This holds the needed information to create each function later
		 * on. Also acts as an intermediate store for types until we have found all types in the
		 * current package. Otherwise, we would behave like C, that the type declarations have to be
		 * before anything that uses them.
		 */
		class FunctionDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR FunctionDecl(Par<SyntaxEnv> env,
									Par<TypeName> result,
									Par<SStr> name,
									Par<Params> params,
									Par<SStr> contents);

			STORM_CTOR FunctionDecl(Par<SyntaxEnv> env,
									Par<TypeName> result,
									Par<SStr> name,
									Par<Params> params,
									Par<TypeName> thread,
									Par<SStr> contents);

			// Values.
			STORM_VAR Auto<SyntaxEnv> env;
			STORM_VAR Auto<SStr> name;
			STORM_VAR Auto<TypeName> result;
			STORM_VAR Auto<Params> params;
			STORM_VAR MAYBE(Auto<TypeName>) thread;
			STORM_VAR Auto<SStr> contents;

			// Create a corresponding function.
			Function *STORM_FN createFn();

			// Temporary solution for updating a function.
			void STORM_FN update(Par<BSFunction> fn);

			// Get our name as a NamePart.
			NamePart *STORM_FN namePart() const;
		};

		/**
		 * Raw function that will not read syntax trees by parsing a string.
		 */
		class BSRawFn : public Function {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSRawFn(Value result, Par<SStr> name, Par<Array<Value>> params,
							Par<ArrayP<Str>> paramNames, MAYBE(Par<NamedThread>) thread);

			// Create (more c++-like). NOTE: 'thread' is not needed, but due to a bug in
			// StormBuiltin, it can not be marked as MAYBE().
			BSRawFn(Value result, Par<SStr> name, const vector<Value> &params, const vector<String> &names,
					Par<NamedThread> thread);

			// Declared at.
			SrcPos pos;

			// Override this to create the syntax tree to compile.
			virtual FnBody *STORM_FN createBody();

			// Re-compile at next execution.
			void STORM_FN reset();

			// Add function parameters to a block. Mainly for internal use.
			void STORM_FN addParams(Par<Block> block);

		protected:
			// Parameter names.
			vector<String> paramNames;

		private:
			// Generate code.
			CodeGen *CODECALL generateCode();

			// Initialize.
			void init(Par<NamedThread> thread);
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

			// Scope.
			const Scope scope;

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
		 * Function using a pre-created syntax tree.
		 */
		class BSTreeFn : public BSRawFn {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSTreeFn(Value result, Par<SStr> name, Par<Array<Value>> params,
								Par<ArrayP<Str>> names, MAYBE(Par<NamedThread>) thread);

			// Set the root of the syntax tree. Resetting the body also makes the function re-compile.
			void STORM_FN body(Par<FnBody> body);

			// Override to use the body.
			virtual FnBody *STORM_FN createBody();

		private:
			// Body.
			Auto<FnBody> root;
		};


		/**
		 * Contents of a function.
		 */
		class FnBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR FnBody(Par<BSRawFn> owner, Scope scope);
			STORM_CTOR FnBody(Par<BSFunction> owner);

			// Store the result type.
			STORM_VAR Value type;
		};

	}
}
