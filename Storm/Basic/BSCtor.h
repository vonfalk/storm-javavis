#pragma once
#include "Code/Listing.h"
#include "Function.h"
#include "SyntaxObject.h"
#include "BSBlock.h"
#include "Lib/Array.h"

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
			// Create. If contents is null, we will generate the default ctor.
			BSCtor(const vector<Value> &values, const vector<String> &names, Par<Class> owner,
				const Scope &scope, Par<SStr> contents, const SrcPos &pos);

			// Scope.
			const Scope scope;

			// Index of the parameter used to store the thread we're supposed to run on. 'invalid' otherwise.
			nat threadParam;

			// Invalid parameter id.
			static const nat invalidParam;

			// We may need to run on a specific thread based on the current actual parameters.
			virtual code::Variable findThread(const GenState &s, const Actuals &params);

			// Add parameters. Returns the local variable that represents the 'threadParam' above.
			LocalVar *addParams(Par<Block> to);

		private:
			// Parameter names.
			vector<String> paramNames;

			// Contents
			Auto<SStr> contents;

			// Position.
			SrcPos pos;

			// Generate code.
			code::Listing generateCode();

			// Parse.
			CtorBody *parse();

			// Default ctor contents.
			CtorBody *defaultParse();
		};

		/**
		 * Contents of the constructor.
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
			virtual void blockCode(const GenState &state, GenResult &to, const code::Block &block);
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
			STORM_CTOR SuperCall(Par<CtorBody> block, Par<Actual> params);
			STORM_CTOR SuperCall(Par<CtorBody> block, Par<Actual> params, Par<ArrayP<Initializer>> init);

			// Result.
			virtual Value result();

			// Code.
			virtual void code(const GenState &s, GenResult &r);

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
			void callParent(const GenState &s);

			// Call the TObject's ctor.
			void callTObject(const GenState &s);

			// Figure out which thread we want to run on!
			code::Value tObjectThread();

			// Initialize a variable.
			void initVar(const GenState &s, Par<TypeVar> var);
			void initVar(const GenState &s, Par<TypeVar> var, Par<Initializer> to);
			void initVarCtor(const GenState &s, Par<TypeVar> var, Par<Actual> to);
			void initVarAssign(const GenState &s, Par<TypeVar> var, Par<Expr> to);

			// Initialize a variable with its default constructor.
			void initVarDefault(const GenState &s, Par<TypeVar> var);

			// Add an initializer.
			void init(Par<Initializer> init);
		};

	}
}
