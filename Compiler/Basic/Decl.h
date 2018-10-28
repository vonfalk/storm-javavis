#pragma once
#include "Core/TObject.h"
#include "Core/Thread.h"
#include "Compiler/Named.h"
#include "Compiler/Visibility.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Declaration that will be expanded later on.
		 *
		 * Generated by the parser in cases where we need to delay the creation of some entity
		 */
		class NamedDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR NamedDecl();

			// Visibility (if set).
			MAYBE(Visibility *) visibility;

			// Location of the documentation (if set).
			SrcPos docPos;

			// Create the actual named entity.
			Named *STORM_FN create();

			// Initialize the declaration previously created (if any).
			void STORM_FN resolve();

			// Update this declaration (if possible). If no previous implementation was found, a
			// newly created instance of this element is returned. If something is returned from
			// here, we don't expect 'resolve' to be called later.
			//
			// Note: This function will likely be removed in the future, when we have a more generic
			// mechanism for updating functions.
			virtual MAYBE(Named *) STORM_FN update(Scope scope);

		protected:
			// Do the actual creation.
			virtual Named *STORM_FN doCreate();

			// Resolve things inside 'named'.
			virtual void STORM_FN doResolve(Named *entity);

		private:
			// Previously created entity.
			MAYBE(Named *) created;
		};

	}
}
