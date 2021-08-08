#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Template.h"
#include "Compiler/Visibility.h"
#include "Decl.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class UseThreadDecl;


		/**
		 * Content of a single source file.
		 */
		class Content : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Content();

			// Add a type.
			void STORM_FN add(Type *type);

			// Add a declaration of a named entity.
			void STORM_FN add(NamedDecl *fn);

			// Add a named thread.
			void STORM_FN add(NamedThread *thread);

			// Add a template of some kind.
			void STORM_FN add(Template *templ);

			// Set the default access modifier.
			void STORM_FN add(Visibility *v);

			// Set the default thread for subsequent entries.
			void STORM_FN add(UseThreadDecl *t);

			// Add either of the above types.
			void STORM_FN add(TObject *obj);

			// All types.
			Array<Type *> *types;

			// All declarations of named objects.
			Array<NamedDecl *> *decls;

			// All named threads.
			Array<NamedThread *> *threads;

			// All templates.
			Array<Template *> *templates;

			// Default access modifier.
			Visibility *defaultVisibility;

			// Default thread to use.
			MAYBE(SrcName *) defaultThread;

		private:
			// Apply visibility to named objects.
			void update(Named *named);
			void update(NamedDecl *fn);
		};


		/**
		 * Declaration that expands to multiple declarations.
		 *
		 * Used since it is not possible to pass Array<TObject *> directly to Content as it is an
		 * Object, not a TObject.
		 */
		class MultiDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR MultiDecl();
			STORM_CTOR MultiDecl(Array<TObject *> *data);

			Array<TObject *> *data;

			void STORM_FN push(TObject *v);
		};


		/**
		 * Declare that a specific thread should be applied to all functions/classes in this file.
		 */
		class UseThreadDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR UseThreadDecl(SrcName *name);

			// The name of the thread to use.
			SrcName *thread;

			// To string.
			void STORM_FN toS(StrBuf *to) const;
		};

	}
}
