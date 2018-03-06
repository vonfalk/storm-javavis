#pragma once
#include "Core/Map.h"
#include "Core/Array.h"
#include "Compiler/Type.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Scope.h"
#include "Compiler/Visibility.h"
#include "Param.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BSFunction;
		class BSCtor;

		class Class : public Type {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Class(TypeFlags flags, SrcPos pos, Scope scope, Str *name, syntax::Node *body);

			// The scope used for this class.
			Scope scope;

			// Lookup any additional types needed.
			void lookupTypes();

			// Declared at.
			SrcPos declared;

			// Base class (if any).
			Name *base;

			// Associated thread (if any).
			Name *thread;

		protected:
			// Load the body lazily.
			virtual Bool STORM_FN loadAll();

		private:
			// Body of the class.
			syntax::Node *body;

			// Allowing lazy loads?
			bool allowLazyLoad;
		};


		// Create a class not extended from anything.
		Class *STORM_FN createClass(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body);
		Class *STORM_FN createValue(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body);

		// Create a class extended from a base class.
		Class *STORM_FN extendClass(SrcPos pos, Scope env, syntax::SStr *name, Name *from, syntax::Node *body);
		Class *STORM_FN extendValue(SrcPos pos, Scope env, syntax::SStr *name, Name *from, syntax::Node *body);

		// Create an actor class.
		Class *STORM_FN threadClass(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body);
		Class *STORM_FN threadClass(SrcPos pos, Scope env, syntax::SStr *name, Name *thread, syntax::Node *body);


		/**
		 * Class body.
		 */
		class ClassBody : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody();

			// Add content.
			void STORM_FN add(Named *item);

			// Add template.
			void STORM_FN add(Template *t);

			// Add default access modifier.
			void STORM_FN add(Visibility *v);

			// Add any of the above things.
			void STORM_FN add(TObject *t);

			// Contents.
			Array<Named *> *items;

			// Template contents.
			Array<Template *> *templates;

			// Currently default access modifier.
			Visibility *defaultVisibility;
		};


		/**
		 * Class variable.
		 */
		class ClassVar : public MemberVar {
			STORM_CLASS;
		public:
			STORM_CTOR ClassVar(Class *owner, SrcName *type, syntax::SStr *name);
		};

		/**
		 * Class function.
		 */
		BSFunction *STORM_FN classFn(Class *owner,
									SrcPos pos,
									syntax::SStr *name,
									Name *result,
									Array<NameParam> *params,
									syntax::Node *content);

		/**
		 * Class function declared as 'assign function'.
		 */
		BSFunction *STORM_FN classAssign(Class *owner,
										SrcPos pos,
										syntax::SStr *name,
										Array<NameParam> *params,
										syntax::Node *content);

		/**
		 * Class constructor.
		 */
		BSCtor *STORM_FN classCtor(Class *owner,
								SrcPos pos,
								Array<NameParam> *params,
								syntax::Node *content);

		BSCtor *STORM_FN classCastCtor(Class *owner,
									SrcPos pos,
									Array<NameParam> *params,
									syntax::Node *content);

		BSCtor *STORM_FN classDefaultCtor(Class *owner);

	}
}
