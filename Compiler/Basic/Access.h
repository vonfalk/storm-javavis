#pragma once
#include "Core/EnginePtr.h"
#include "Compiler/Visibility.h"
#include "Core/SrcPos.h"
#include "Decl.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class MemberWrap;

		/**
		 * Access modifiers in Basic Storm. We divide them into two categories: type access and free
		 * access. The type access are used within types, while the free access modifiers are used
		 * outside of types. The access modifiers within types have slightly different meanings in
		 * some cases. Most notably 'private', which means 'private within this file' for free
		 * functions and 'private within this type' for types.
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
		NamedDecl *STORM_FN apply(SrcPos pos, NamedDecl *to, Visibility *v);
		MemberWrap *STORM_FN apply(SrcPos pos, MemberWrap *wrap, Visibility *v);

		// Apply to unknown objects. Currently, Named objects, FunctionDecl and MemberWrap are supported.
		TObject *STORM_FN apply(SrcPos pos, TObject *to, Visibility *v);

	}
}
