#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Type.h"
#include "TypeVar.h"

#include "BSType.h"
#include "BSParams.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BSFunction;

		class Class : public Type {
			STORM_CLASS;
		public:
			// Create class 'name' with contents 'content'.
			STORM_CTOR Class(SrcPos pos, Par<SStr> name, Par<SStr> content);

			// Create class 'name' with contents 'content', extended from 'base'.
			STORM_CTOR Class(SrcPos pos, Par<SStr> name, Par<SStr> content, Par<TypeName> base);

			// The scope used for this class.
			Scope scope;

			// Set the scope (done by BSContents).
			void setScope(const Scope &scope);

			// Set the base class.
			void setBase();

			// Declared at.
			const SrcPos declared;

		protected:
			// Load the contents lazily.
			virtual void lazyLoad();

		private:
			// Contents of the class.
			Auto<SStr> content;

			// Base class (if any).
			Auto<TypeName> base;
		};


		/**
		 * Class body.
		 */
		class ClassBody : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody();

			// Add content.
			void STORM_FN add(Par<NameOverload> item);

			// Contents.
			vector<Auto<NameOverload> > items;
		};


		/**
		 * Class variable.
		 */
		class ClassVar : public TypeVar {
			STORM_CLASS;
		public:
			STORM_CTOR ClassVar(Par<Class> owner, Par<TypeName> type, Par<SStr> name);
		};

		/**
		 * Class function.
		 */
		BSFunction *STORM_FN classFn(Par<Class> owner,
									SrcPos pos,
									Par<SStr> name,
									Par<TypeName> result,
									Par<Params> params,
									Par<SStr> contents);

	}
}
