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
		class BSCtor;

		class Class : public Type {
			STORM_CLASS;
		public:
			// Create.
			Class(TypeFlags flags, const SrcPos &pos, const String &name, Par<SStr> content);

			// The scope used for this class.
			Scope scope;

			// Set the scope (done by BSContents).
			void setScope(const Scope &scope);

			// Lookup any additional types needed.
			void lookupTypes();

			// Declared at.
			const SrcPos declared;

			// Base class (if any).
			Auto<TypeName> base;

			// Associated thread (if any).
			Auto<TypeName> thread;

			// Load the contents lazily.
			virtual bool loadAll();

		private:
			// Contents of the class.
			Auto<SStr> content;

			// Allowing lazy loads?
			bool allowLazyLoad;
		};


		// Create a class not extended from anything.
		Class *STORM_FN createClass(SrcPos pos, Par<SStr> name, Par<SStr> content);
		Class *STORM_FN createValue(SrcPos pos, Par<SStr> name, Par<SStr> content);

		// Create a class extended from a base class.
		Class *STORM_FN extendClass(SrcPos pos, Par<SStr> name, Par<TypeName> from, Par<SStr> content);
		Class *STORM_FN extendValue(SrcPos pos, Par<SStr> name, Par<TypeName> from, Par<SStr> content);

		// Create an actor class.
		Class *STORM_FN threadClass(SrcPos pos, Par<SStr> name, Par<SStr> content);
		Class *STORM_FN threadClass(SrcPos pos, Par<SStr> name, Par<TypeName> thread, Par<SStr> content);


		/**
		 * Class body.
		 */
		class ClassBody : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody();

			// Add content.
			void STORM_FN add(Par<Named> item);

			// Contents.
			vector<Auto<Named> > items;
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

		/**
		 * Class constructor.
		 */
		BSCtor *STORM_FN classCtor(Par<Class> owner,
									SrcPos pos,
									Par<Params> params,
									Par<SStr> contents);

		BSCtor *STORM_FN classCastCtor(Par<Class> owner,
									SrcPos pos,
									Par<Params> params,
									Par<SStr> contents);

		BSCtor *STORM_FN classDefaultCtor(Par<Class> owner);

	}
}
