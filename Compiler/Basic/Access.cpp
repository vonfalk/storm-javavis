#include "stdafx.h"
#include "Access.h"
#include "Engine.h"
#include "Exception.h"

namespace storm {
	namespace bs {

		Visibility *typePublic(EnginePtr e) {
			return e.v.visibility(Engine::vPublic);
		}

		Visibility *typeProtected(EnginePtr e) {
			return e.v.visibility(Engine::vTypeProtected);
		}

		Visibility *typePackage(EnginePtr e) {
			return e.v.visibility(Engine::vPackagePrivate);
		}

		Visibility *typePrivate(EnginePtr e) {
			return e.v.visibility(Engine::vTypePrivate);
		}

		Visibility *freePublic(EnginePtr e) {
			return e.v.visibility(Engine::vPublic);
		}

		Visibility *freePackage(EnginePtr e) {
			return e.v.visibility(Engine::vPackagePrivate);
		}

		Visibility *freePrivate(EnginePtr e) {
			throw RuntimeError(L"The keyword 'private' for non-members is not implemented yet.");
			return e.v.visibility(Engine::vPublic);
		}


		Named *apply(SrcPos pos, Named *to, Visibility *v) {
			if (to->visibility)
				throw SyntaxError(pos, ::toS(to->name) +
								L" already has a visibility specified. Can not add " +
								::toS(v) + L" as well.");

			to->visibility = v;
			return to;
		}

		FunctionDecl *apply(SrcPos pos, FunctionDecl *to, Visibility *v) {
			if (to->visibility)
				throw SyntaxError(pos, L"The function " + ::toS(to->name) +
								L" already has a visibility specified. Can not add " +
								::toS(v) + L" as well.");

			to->visibility = v;
			return to;
		}

		TObject *apply(SrcPos pos, TObject *to, Visibility *v) {
			if (Named *n = as<Named>(to))
				return apply(pos, n, v);
			else if (FunctionDecl *d = as<FunctionDecl>(to))
				return apply(pos, d, v);
			else
				throw InternalError(L"I can not apply visibility to " + ::toS(to));
		}

	}
}
