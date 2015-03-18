#pragma once
#include "Code/Listing.h"
#include "Function.h"
#include "SyntaxObject.h"
#include "BSBlock.h"
#include "Lib/Array.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A constructor. Enforces that the parent constructor is called.
		 */
		class BSCtor : public Function {
			STORM_CLASS;
		public:
			// Create.
			BSCtor(const vector<Value> &values, const vector<String> &names,
				const Scope &scope, Par<SStr> contents, const SrcPos &pos);

			// Scope.
			const Scope scope;

			// Add parameters.
			void addParams(Par<Block> to);

		private:
			// Parameter names.
			vector<String> paramNames;

			// Contents
			Auto<SStr> contents;

			// Position.
			SrcPos pos;

			// Generate code.
			code::Listing generateCode();
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

			// Initialize to.
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
			virtual void code(GenState &s, GenResult &r);

		private:
			// Member of.
			Value thisPtr;

			// Actual parameters.
			Auto<Actual> params;

			// This parameter.
			Auto<LocalVar> thisVar;

			// Scope.
			Scope scope;

			// Initializers.
			typedef hash_map<String, Auto<Actual>> InitMap;
			InitMap initMap;

			// Init.
			void init(Par<CtorBody> block, Par<Actual> params);

			// Call the parent constructor (if any).
			void callParent(GenState &s);

			// Initialize a variable.
			void initVar(GenState &s, Par<TypeVar> var);
			void initVar(GenState &s, Par<TypeVar> var, Par<Actual> to);

			// Initialize a variable with its default constructor.
			void initVarDefault(GenState &s, Par<TypeVar> var);

			// Add an initializer.
			void init(Par<Initializer> init);
		};

	}
}
