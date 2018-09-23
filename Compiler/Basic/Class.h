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

		class BSRawFn;
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

			// State used to keep track of what is added.
			struct AddState {
				AddState();

				bool ctor;
				bool copyCtor;
				bool deepCopy;
				bool assign;
			};

			// Add a member to the class, keeping track of what already exists.
			void addMember(Named *member, AddState &s);
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
		 * Wrapper around syntax nodes for class members so that we can attach a visibilty to them.
		 */
		class MemberWrap : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR MemberWrap(syntax::Node *node);

			// Node we're wrapping.
			syntax::Node *node;

			// Visibility of this node (if attached).
			MAYBE(Visibility *) visibility;

			// If set: location of the documentation for this member.
			SrcPos docPos;

			// Transform this node.
			virtual Named *STORM_FN transform(Class *owner);
		};


		/**
		 * Class body.
		 */
		class ClassBody : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR ClassBody(Class *owner);

			// Add content.
			virtual void STORM_FN add(Named *item);

			// Add a wrapped syntax node.
			virtual void STORM_FN add(MemberWrap *wrap);

			// Add template.
			virtual void STORM_FN add(Template *t);

			// Add default access modifier.
			virtual void STORM_FN add(Visibility *v);

			// Add any of the above things.
			virtual void STORM_FN add(TObject *t);

			// Called before the class attempts to extract data from the class. Allows extensions to
			// get a last chance at altering the data before we process it.
			virtual void STORM_FN prepareItems();

			// Called after 'item' is loaded but before 'wraps' are loaded.
			virtual void STORM_FN prepareWraps();

			// Owning class.
			Class *owner;

			// Contents.
			Array<Named *> *items;

			// Wrapped members for later evaluation.
			Array<MemberWrap *> *wraps;

			// Template contents.
			Array<Template *> *templates;

			// Currently default access modifier.
			Visibility *defaultVisibility;
		};


		/**
		 * Class variable.
		 */
		MemberVar *STORM_FN classVar(Class *owner, SrcName *type, syntax::SStr *name) ON(Compiler);

		/**
		 * Class function.
		 */
		BSFunction *STORM_FN classFn(Class *owner,
									SrcPos pos,
									syntax::SStr *name,
									Name *result,
									Array<NameParam> *params,
									syntax::Node *content) ON(Compiler);

		/**
		 * Abstract function in a class.
		 */
		Function *STORM_FN classAbstractFn(Class *owner,
										SrcPos pos,
										syntax::SStr *name,
										Name *result,
										Array<NameParam> *params,
										syntax::Node *options) ON(Compiler);

		/**
		 * Class function declared as 'assign function'.
		 */
		BSFunction *STORM_FN classAssign(Class *owner,
										SrcPos pos,
										syntax::SStr *name,
										Array<NameParam> *params,
										syntax::Node *content) ON(Compiler);

		/**
		 * Class constructor.
		 */
		BSCtor *STORM_FN classCtor(Class *owner,
								SrcPos pos,
								Array<NameParam> *params,
								syntax::Node *content) ON(Compiler);

		BSCtor *STORM_FN classCastCtor(Class *owner,
									SrcPos pos,
									Array<NameParam> *params,
									syntax::Node *content) ON(Compiler);

		BSCtor *STORM_FN classDefaultCtor(Class *owner) ON(Compiler);


		// Make a function static if it was previously a member function.
		void STORM_FN makeStatic(Class *owner, BSRawFn *fn);

	}
}
