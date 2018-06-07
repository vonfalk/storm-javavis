#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Doc.h"
#include "Param.h"
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class FnBody;
		class BSFunction;

		/**
		 * Function declaration. This holds the needed information to create each function later
		 * on. Also acts as an intermediate store for types until we have found all types in the
		 * current package. Otherwise, we would behave like C, that the type declarations have to be
		 * before anything that uses them.
		 *
		 * If 'result' is null, then we assume that this function declares a setter function, since
		 * the return type is always 'null' for them.
		 */
		class FunctionDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR FunctionDecl(Scope scope,
									MAYBE(SrcName *) result,
									syntax::SStr *name,
									Array<NameParam> *params,
									syntax::Node *body);

			STORM_CTOR FunctionDecl(Scope scope,
									MAYBE(SrcName *) result,
									syntax::SStr *name,
									Array<NameParam> *params,
									SrcName *thread,
									syntax::Node *body);

			// Values.
			Scope scope;
			syntax::SStr *name;
			MAYBE(SrcName *) result;
			Array<NameParam> *params;
			MAYBE(SrcName *) thread;
			syntax::Node *body;
			MAYBE(Visibility *) visibility;

			// Position of the documentation. May be unset.
			SrcPos docPos;

			// Create the corresponding function.
			Function *STORM_FN createFn();

			// Temporary solution for updating a function.
			void STORM_FN update(BSFunction *fn);

			// Get our name as a NamePart.
			NamePart *STORM_FN namePart() const;
		};

		// Declare setter functions.
		FunctionDecl *STORM_FN assignDecl(Scope scope,
										syntax::SStr *name,
										Array<NameParam> *params,
										syntax::Node *body);

		FunctionDecl *STORM_FN assignDecl(Scope scope,
										syntax::SStr *name,
										Array<NameParam> *params,
										SrcName *thread,
										syntax::Node *body);


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

			// Add function parameters to a block. Mainly for internal use.
			void STORM_FN addParams(Block *block);

			// Get parameters as required by documentation.
			Array<DocParam> *STORM_FN docParams();

		protected:
			// Parameter names and values.
			Array<ValParam> *valParams;

			// Re-compile at next execution.
			void STORM_FN reset();

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

			// Temporary solution for updating a function.
			Bool STORM_FN update(Array<ValParam> *params, syntax::Node *body, SrcPos pos);
			Bool STORM_FN update(Array<ValParam> *params, syntax::Node *body);
			Bool STORM_FN update(Array<Str *> *params, syntax::Node *body);
			Bool STORM_FN update(BSFunction *from);

		private:
			// Code.
			MAYBE(syntax::Node *) body;
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
			void STORM_ASSIGN body(FnBody *body);

			// Override to use the body.
			virtual FnBody *STORM_FN createBody();

		private:
			// Body.
			MAYBE(FnBody *) root;
		};


		/**
		 * Abstract function that throws an exception when invoked.
		 */
		class BSAbstractFn : public BSRawFn {
			STORM_CLASS;
		public:
			STORM_CTOR BSAbstractFn(Value result, syntax::SStr *name, Array<ValParam> *params);

			// Override to use the body.
			virtual FnBody *STORM_FN createBody();
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
