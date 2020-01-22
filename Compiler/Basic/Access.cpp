#include "stdafx.h"
#include "Access.h"
#include "Engine.h"
#include "Exception.h"
#include "Class.h"
#include "Function.h"
#include "Scope.h"

namespace storm {
	namespace bs {

		static SrcPos findPos(NameLookup *src) {
			while (src) {
				if (BSRawFn *fn = as<BSRawFn>(src))
					return fn->pos;
				else if (Class *c = as<Class>(src))
					return c->declared;
				else if (BlockLookup *b = as<BlockLookup>(src))
					return b->block->pos;
				else if (FileScope *f = as<FileScope>(src))
					return f->pos;

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
			if (to->visibility) {
				Str *msg = TO_S(to, to->name << S(" already has a visibility specified. Can not add ")
								<< v << S(" as well."));
				throw new (to) SyntaxError(pos, msg);
			}

			to->visibility = v;
			return to;
		}

		NamedDecl *apply(SrcPos pos, NamedDecl *to, Visibility *v) {
			if (to->visibility) {
				Str *msg = TO_S(to, S("The declaration ") << to
								<< S(" already has a visibility specified. Can not add ")
								<< v << S(" as well."));
				throw new (to) SyntaxError(pos, msg);
			}

			to->visibility = v;
			return to;
		}

		MemberWrap *apply(SrcPos pos, MemberWrap *to, Visibility *v) {
			if (to->visibility)
				throw new (to) SyntaxError(pos, S("This member already has a visibility specified. Can not add another."));

			to->visibility = v;
			return to;
		}

		TObject *apply(SrcPos pos, TObject *to, Visibility *v) {
			if (Named *n = as<Named>(to))
				return apply(pos, n, v);
			else if (NamedDecl *d = as<NamedDecl>(to))
				return apply(pos, d, v);
			else if (MemberWrap *wrap = as<MemberWrap>(to))
				return apply(pos, wrap, v);
			else
				throw new (to) InternalError(TO_S(to, S("I can not apply visibility to ") << to));
		}

	}
}
