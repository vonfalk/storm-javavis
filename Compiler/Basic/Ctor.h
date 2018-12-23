#pragma once
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/Array.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Variable.h"
#include "Block.h"
#include "Param.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class CtorBody;
		class Class;

		/**
		 * Raw constructor call, much like BSRawFn.
		 */
		class BSRawCtor : public Function {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSRawCtor(Array<ValParam> *params, SrcPos pos);

			// Position.
			SrcPos pos;

			// We may need to run on a specific thread based on the current actual parameters.
			virtual code::Var STORM_FN findThread(CodeGen *s, Array<code::Operand> *params);

			// Add parameters. Returns the local variable that represents the 'threadParam' above if needed.
			LocalVar *addParams(Block *to);

			// Create the body.
			virtual CtorBody *STORM_FN createBody();

		protected:
			// Re-compile at next execution.
			void STORM_FN reset();

		private:
			// Parameter names.
			Array<ValParam> *params;

			// Generate code.
			CodeGen *CODECALL generateCode();

			// Do we need a 'hidden' thread parameter?
			bool needsThread;
		};


		/**
		 * A constructor. Enforces that the parent constructor is called.
		 */
		class BSCtor : public BSRawCtor {
			STORM_CLASS;
		public:
			// Create. If body is null, we will generate the default ctor.
			BSCtor(Array<ValParam> *params, Scope scope, syntax::Node *body, SrcPos pos);

			// Scope.
			Scope scope;

			// Body.
			MAYBE(syntax::Node *) body;

			// Create the body.
			virtual CtorBody *STORM_FN createBody();

		private:
			// Default ctor body.
			CtorBody *defaultParse();
		};


		/**
		 * A constructor using a pre-created syntax tree.
		 */
		class BSTreeCtor : public BSRawCtor {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSTreeCtor(Array<ValParam> *params, SrcPos pos);

			// Set the root of the syntax tree. Resetting the body also makes the function re-compile.
			void STORM_ASSIGN body(CtorBody *body);

			// Create the body.
			virtual CtorBody *STORM_FN createBody();

		private:
			// Body.
			MAYBE(CtorBody *) root;
		};


		/**
		 * Body of the constructor.
		 */
		class CtorBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR CtorBody(BSCtor *owner);
			STORM_CTOR CtorBody(BSRawCtor *owner, Scope scope);

			using ExprBlock::add;
			void STORM_FN add(Array<Expr *> *exprs);

			// Temporary storage of the actual LocalVar that stores the parameter we need to capture.
			LocalVar *threadParam;

			// Stores a copy of the parameter used to store the thread. Not reference counted.
			// This is to make it impossible to overwrite the thread parameter before passing it
			// to the TObject's constructor, like this:
			// ctor(Thread a) { a = x; init(); }.
			code::Var thread;

			// Code generation.
			virtual void blockCode(CodeGen *state, CodeResult *to, const code::Block &block);
		};

		/**
		 * Initializer.
		 */
		class Initializer : public Object {
			STORM_CLASS;
		public:
			// Assignment initialization.
			STORM_CTOR Initializer(syntax::SStr *name, Expr *expr);

			// Custom constructor.
			STORM_CTOR Initializer(syntax::SStr *name, Actuals *params);

			// Name of the variable.
			syntax::SStr *name;

			// Initialize by assignment (may be null).
			MAYBE(Expr *) expr;

			// Initialize to (may be null).
			MAYBE(Actuals *) params;

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * Call the super class.
		 */
		class SuperCall : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR SuperCall(SrcPos pos, CtorBody *block, Actuals *params);
			STORM_CTOR SuperCall(SrcPos pos, CtorBody *block, Actuals *params, Array<Initializer *> *init);

			// Add a new initializer.
			void STORM_FN init(Initializer *init);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Member of.
			Value thisPtr;

			// Actual parameters.
			Actuals *params;

			// This parameter.
			LocalVar *thisVar;

			// Body.
			CtorBody *rootBlock;

			// Scope.
			Scope scope;

			// Initializers. Remember the order so that we can initialize members in the order they
			// appear in the source code.
			Array<Initializer *> *initializers;

			// Remember which variables have initializers.
			Set<Str *> *initialized;

			// Init.
			void init(CtorBody *block, Actuals *params);

			// Call the parent constructor (if any).
			void callParent(CodeGen *s);

			// Call the TObject's ctor, assuming we want to run on 't'.
			void callTObject(CodeGen *s, NamedThread *t);

			// Initialize a variable.
			void initVar(CodeGen *s, MemberVar *var, Initializer *to);
			void initVarCtor(CodeGen *s, MemberVar *var, Actuals *to);
			void initVarAssign(CodeGen *s, MemberVar *var, Expr *to);

			// Initialize a variable with its default constructor.
			void initVarDefault(CodeGen *s, MemberVar *var);
		};

	}
}
