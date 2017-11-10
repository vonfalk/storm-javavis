#pragma once
#include "Core/Map.h"
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
		 * A constructor. Enforces that the parent constructor is called.
		 */
		class BSCtor : public Function {
			STORM_CLASS;
		public:
			// Create. If body is null, we will generate the default ctor.
			BSCtor(Array<ValParam> *params, Scope scope, syntax::Node *body, SrcPos pos);

			// Scope.
			Scope scope;

			// Position.
			SrcPos pos;

			// We may need to run on a specific thread based on the current actual parameters.
			virtual code::Var STORM_FN findThread(CodeGen *s, Array<code::Operand> *params);

			// Add parameters. Returns the local variable that represents the 'threadParam' above if needed.
			LocalVar *addParams(Block *to);

		private:
			// Parameter names.
			Array<ValParam> *params;

			// Body
			syntax::Node *body;

			// Generate code.
			CodeGen *CODECALL generateCode();

			// Parse.
			CtorBody *parse();

			// Default ctor body.
			CtorBody *defaultParse();

			// Do we need a 'hidden' thread parameter?
			bool needsThread;
		};

		/**
		 * Body of the constructor.
		 */
		class CtorBody : public ExprBlock {
			STORM_CLASS;
		public:
			STORM_CTOR CtorBody(BSCtor *owner);

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
			Expr *expr;

			// Initialize to (may be null).
			Actuals *params;
		};

		/**
		 * Call the super class.
		 */
		class SuperCall : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR SuperCall(SrcPos pos, CtorBody *block, Actuals *params);
			STORM_CTOR SuperCall(SrcPos pos, CtorBody *block, Actuals *params, Array<Initializer *>*init);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

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

			// Initializers.
			typedef Map<Str *, Initializer *> InitMap;
			Map<Str *, Initializer *> *initMap;

			// Init.
			void init(CtorBody *block, Actuals *params);

			// Call the parent constructor (if any).
			void callParent(CodeGen *s);

			// Call the TObject's ctor, assuming we want to run on 't'.
			void callTObject(CodeGen *s, NamedThread *t);

			// Initialize a variable.
			void initVar(CodeGen *s, MemberVar *var);
			void initVar(CodeGen *s, MemberVar *var, Initializer *to);
			void initVarCtor(CodeGen *s, MemberVar *var, Actuals *to);
			void initVarAssign(CodeGen *s, MemberVar *var, Expr *to);

			// Initialize a variable with its default constructor.
			void initVarDefault(CodeGen *s, MemberVar *var);

			// Add an initializer.
			void init(Initializer *init);
		};

	}
}
