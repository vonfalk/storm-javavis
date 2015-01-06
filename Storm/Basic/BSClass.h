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
			STORM_CTOR Class(SrcPos pos, Auto<SStr> name, Auto<SStr> content);

			// The scope used for this class.
			Scope scope;

			// Set the scope (done by BSContents).
			void setScope(const Scope &scope);

			// Declared at.
			const SrcPos declared;

		protected:
			// Load the contents lazily.
			virtual void lazyLoad();

		private:
			// Contents of the class.
			Auto<SStr> content;
		};


		/**
		 * Class body.
		 */
		class ClassBody : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody();

			// Add content.
			void STORM_FN add(Auto<NameOverload> item);

			// Contents.
			vector<Auto<NameOverload> > items;
		};


		/**
		 * Class variable.
		 */
		class ClassVar : public TypeVar {
			STORM_CLASS;
		public:
			STORM_CTOR ClassVar(Auto<Class> owner, Auto<TypeName> type, Auto<SStr> name);
		};

		/**
		 * Class function.
		 */
		BSFunction *STORM_FN classFn(Auto<Class> owner,
									SrcPos pos,
									Auto<SStr> name,
									Auto<TypeName> result,
									Auto<Params> params,
									Auto<SStr> contents);

	}
}
