#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Type.h"
#include "TypeVar.h"
#include "SyntaxEnv.h"
#include "Shared/Map.h"

#include "BSParams.h"
#include "BSTemplate.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BSFunction;
		class BSCtor;

		class Class : public Type {
			STORM_CLASS;
		public:
			// Create.
			Class(TypeFlags flags, const SrcPos &pos, const Scope &scope, const String &name, Par<SStr> content);

			// The scope used for this class.
			STORM_VAR Scope scope;

			// Lookup any additional types needed.
			void lookupTypes();

			// Declared at.
			const SrcPos declared;

			// Base class (if any).
			Auto<Name> base;

			// Associated thread (if any).
			Auto<Name> thread;

			// Load the contents lazily.
			virtual Bool STORM_FN loadAll();

		private:
			// Contents of the class.
			Auto<SStr> content;

			// Allowing lazy loads?
			bool allowLazyLoad;
		};


		// Create a class not extended from anything.
		Class *STORM_FN createClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<SStr> content);
		Class *STORM_FN createValue(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<SStr> content);

		// Create a class extended from a base class.
		Class *STORM_FN extendClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<Name> from, Par<SStr> content);
		Class *STORM_FN extendValue(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<Name> from, Par<SStr> content);

		// Create an actor class.
		Class *STORM_FN threadClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<SStr> content);
		Class *STORM_FN threadClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<Name> thread, Par<SStr> content);


		/**
		 * Class body.
		 */
		class ClassBody : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody();

			// Add content.
			void STORM_FN add(Par<Named> item);

			// Add template.
			void STORM_FN add(Par<Template> t);

			// Contents.
			STORM_VAR Auto<ArrayP<Named>> items;

			// Template contents. TODO: Add STORM_VAR.
			Auto<MAP_PP(Str, TemplateAdapter)> templates;
		};


		/**
		 * Class variable.
		 */
		class ClassVar : public TypeVar {
			STORM_CLASS;
		public:
			STORM_CTOR ClassVar(Par<Class> owner, Par<SrcName> type, Par<SStr> name);
		};

		/**
		 * Class function.
		 */
		BSFunction *STORM_FN classFn(Par<Class> owner,
									SrcPos pos,
									Par<SStr> name,
									Par<Name> result,
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
