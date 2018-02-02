#include "stdafx.h"
#include "Access.h"
#include "Engine.h"
#include "Exception.h"
#include "Class.h"
#include "Function.h"

namespace storm {
	namespace bs {

		static SrcPos findPos(NameLookup *src) {
			while (src) {
				if (BSRawFn *fn = as<BSRawFn>(src))
					return fn->pos;
				else if (Class *c = as<Class>(src))
					return c->declared;

				src = src->parent();
			}

			// TODO: Implement more types here. It would be nice if all Named had a 'pos' member,
			// since that is usable for all named entities.
			return SrcPos();
		}

		FilePrivate::FilePrivate() {}

		Bool FilePrivate::visible(Named *check, NameLookup *source) {
			SrcPos c = findPos(check);
			SrcPos s = findPos(source);

			// If both come from unknown files, they are probably not from the same file.
			if (!c.file || !s.file)
				return false;

			if (c.file == s.file)
				return true;
			return *c.file == *s.file;
		}

		void FilePrivate::toS(StrBuf *to) const {
			*to << S("private");
		}


		/**
		 * Convenience functions.
		 */

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
			return new (e.v) FilePrivate();
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