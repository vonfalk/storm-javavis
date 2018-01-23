#pragma once
#include "Core/EnginePtr.h"
#include "Compiler/Visibility.h"
#include "Compiler/SrcPos.h"
#include "Function.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Access modifiers in Basic Storm. We divide them into two categories: type access and free
		 * access. The type access are used within types, while the free access modifiers are used
		 * outside of types. The access modifiers within types have slightly different meanings in
		 * some cases. Most notably 'private', which means 'private within this file' for free
		 * functions and 'private within this type' for types.
		 *
		 * TODO: Implement support for 'freePrivate'.
		 */



		/**
		 * Create the access modifiers from the grammar.
		 */

		Visibility *STORM_FN typePublic(EnginePtr e);
		Visibility *STORM_FN typeProtected(EnginePtr e);
		Visibility *STORM_FN typePackage(EnginePtr e);
		Visibility *STORM_FN typePrivate(EnginePtr e);

		Visibility *STORM_FN freePublic(EnginePtr e);
		Visibility *STORM_FN freePackage(EnginePtr e);
		Visibility *STORM_FN freePrivate(EnginePtr e);


		// Apply modifiers to named objects.
		Named *STORM_FN apply(SrcPos pos, Named *to, Visibility *v);
		FunctionDecl *STORM_FN apply(SrcPos pos, FunctionDecl *to, Visibility *v);

		// Apply to unknown objects. Currently, Named objects and FunctionDecl are supported.
		TObject *STORM_FN apply(SrcPos pos, TObject *to, Visibility *v);

	}
}
