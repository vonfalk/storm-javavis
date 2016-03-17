#pragma once
#include "Code/Listing.h"
#include "Function.h"
#include "SyntaxObject.h"
#include "BSBlock.h"
#include "TypeVar.h"
#include "Shared/Array.h"
#include "Syntax/Node.h"

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
			BSCtor(const vector<Value> &values, const vector<String> &names,
				const Scope &scope, Par<syntax::Node> body, const SrcPos &pos);

			// Scope.
			const Scope scope;

			// Position.
			const SrcPos pos;

			// We may need to run on a specific thread based on the current actual parameters.
			virtual code::Variable findThread(Par<CodeGen> s, const Actuals &params);

			// Add parameters. Returns the local variable that represents the 'threadParam' above if needed.
			LocalVar *addParams(Par<Block> to);

		private:
			// Parameter names.
			vector<String> paramNames;

			// Body
			Auto<syntax::Node> body;

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
			STORM_CTOR CtorBody(Par<BSCtor> owner);

			using ExprBlock::add;
			void STORM_FN add(Par<ArrayP<Expr>> exprs);

			// Temporary storage of the actual LocalVar that stores the parameter we need to capture.
			Auto<LocalVar> threadParam;

			// Stores a copy of the parameter used to store the thread. Not reference counted.
			// This is to make it impossible to overwrite the thread parameter before passing it
			// to the TObject's constructor, like this:
			// ctor(Thread a) { a = x; init(); }.
			code::Variable thread;

			// Code generation.
			virtual void blockCode(Par<CodeGen> state, Par<CodeResult> to, const code::Block &block);
		};

		/**
		 * Initializer.
		 */
		class Initializer : public Object {
			STORM_CLASS;
		public:
			// Assignment initialization.
			STORM_CTOR Initializer(Par<SStr> name, Par<Expr> expr);

			// Custom constructor.
			STORM_CTOR Initializer(Par<SStr> name, Par<Actual> params);

			// Name of the variable.
			Auto<SStr> name;

			// Initialize by assignment (may be null).
			Auto<Expr> expr;

			// Initialize to (may be null).
			Auto<Actual> params;
		};

		/**
		 * Call the super class.
		 */
		class SuperCall : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR SuperCall(SrcPos pos, Par<CtorBody> block, Par<Actual> params);
			STORM_CTOR SuperCall(SrcPos pos, Par<CtorBody> block, Par<Actual> params, Par<ArrayP<Initializer>> init);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> r);

		private:
			// Member of.
			Value thisPtr;

			// Actual parameters.
			Auto<Actual> params;

			// This parameter.
			Auto<LocalVar> thisVar;

			// Body.
			CtorBody *rootBlock;

			// Scope.
			Scope scope;

			// Initializers.
			typedef hash_map<String, Auto<Initializer>> InitMap;
			InitMap initMap;

			// Init.
			void init(Par<CtorBody> block, Par<Actual> params);

			// Call the parent constructor (if any).
			void callParent(Par<CodeGen> s);

			// Call the TObject's ctor, assuming we want to run on 't'.
			void callTObject(Par<CodeGen> s, Par<NamedThread> t);

			// Initialize a variable.
			void initVar(Par<CodeGen> s, Par<TypeVar> var);
			void initVar(Par<CodeGen> s, Par<TypeVar> var, Par<Initializer> to);
			void initVarCtor(Par<CodeGen> s, Par<TypeVar> var, Par<Actual> to);
			void initVarAssign(Par<CodeGen> s, Par<TypeVar> var, Par<Expr> to);

			// Initialize a variable with its default constructor.
			void initVarDefault(Par<CodeGen> s, Par<TypeVar> var);

			// Add an initializer.
			void init(Par<Initializer> init);
		};

	}
}
