#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Param.h"
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

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
			STORM_CTOR FunctionDecl(Scope scope,
									SrcName *result,
									syntax::SStr *name,
									Array<NameParam> *params,
									syntax::Node *body);

			STORM_CTOR FunctionDecl(Scope scope,
									SrcName *result,
									syntax::SStr *name,
									Array<NameParam> *params,
									SrcName *thread,
									syntax::Node *body);

			// Values.
			Scope scope;
			syntax::SStr *name;
			SrcName *result;
			Array<NameParam> *params;
			MAYBE(SrcName *) thread;
			syntax::Node *body;

			// Create the corresponding function.
			Function *STORM_FN createFn();
		};


		/**
		 * Raw function that will not read syntax trees by parsing a string.
		 */
		class BSRawFn : public Function {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSRawFn(Value result, syntax::SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread);

			// Declared at.
			SrcPos pos;

			// Override this to create the syntax tree to compile.
			virtual FnBody *STORM_FN createBody();

			// Re-compile at next execution.
			void STORM_FN reset();

			// Add function parameters to a block. Mainly for internal use.
			void STORM_FN addParams(Block *block);

		protected:
			// Parameter names.
			Array<ValParam> *params;

		private:
			// Generate code.
			CodeGen *CODECALL generateCode();

			// Initialize.
			void init(NamedThread *thread);
		};


		/**
		 * Function in the BS language.
		 */
		class BSFunction : public BSRawFn {
			STORM_CLASS;
		public:
			// Create a function.
			STORM_CTOR BSFunction(Value result, syntax::SStr *name, Array<ValParam> *params, Scope scope,
								MAYBE(NamedThread *) thread, syntax::Node *body);

			// Scope.
			Scope scope;

			// Create the body from our string.
			virtual FnBody *STORM_FN createBody();

		private:
			// Code.
			syntax::Node *body;
		};


		/**
		 * Function using a pre-created syntax tree.
		 */
		class BSTreeFn : public BSRawFn {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSTreeFn(Value result, syntax::SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread);

			// Set the root of the syntax tree. Resetting the body also makes the function re-compile.
			void STORM_FN body(FnBody *body);

			// Override to use the body.
			virtual FnBody *STORM_FN createBody();

		private:
			// Body.
			FnBody *root;
		};


		/**
		 * Body of a function.
		 * TODO: FIX INHERITANCE
		 */
		class FnBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR FnBody(BSRawFn *owner, Scope scope);
			STORM_CTOR FnBody(BSFunction *owner);

			// Store the result type (needed for 'return' among others).
			Value type;
		};

	}
}
