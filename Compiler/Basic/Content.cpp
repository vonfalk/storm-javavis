#include "stdafx.h"
#include "Content.h"
#include "Exception.h"
#include "Type.h"
#include "Engine.h"
#include "Class.h"
#include "Core/Str.h"

namespace storm {
	namespace bs {

		UseThreadDecl::UseThreadDecl(SrcName *name) : thread(name) {}

		void UseThreadDecl::toS(StrBuf *to) const {
			*to << L"use " << thread << L":";
		}


		Content::Content() {
			types = new (this) Array<Type *>();
			decls = new (this) Array<NamedDecl *>();
			threads = new (this) Array<NamedThread *>();
			templates = new (this) Array<Template *>();
			defaultVisibility = engine().visibility(Engine::vPublic);
		}

		void Content::add(Type *t) {
			update(t);
			types->push(t);
		}

		void Content::add(NamedDecl *t) {
			update(t);
			decls->push(t);
		}

		void Content::add(NamedThread *t) {
			update(t);
			threads->push(t);
		}

		void Content::add(Template *t) {
			templates->push(t);
		}

		void Content::add(Visibility *v) {
			defaultVisibility = v;
		}

		void Content::add(UseThreadDecl *t) {
			defaultThread = t->thread;
		}

		void Content::add(TObject *o) {
			if (Type *t = as<Type>(o))
				add(t);
			else if (NamedDecl *fn = as<NamedDecl>(o))
				add(fn);
			else if (NamedThread *nt = as<NamedThread>(o))
				add(nt);
			else if (Template *t = as<Template>(o))
				add(t);
			else if (Visibility *v = as<Visibility>(o))
				add(v);
			else if (UseThreadDecl *u = as<UseThreadDecl>(o))
				add(u);
			else
				throw InternalError(L"add for Content does not expect " + ::toS(runtime::typeOf(o)->identifier()));
		}

		void Content::update(Named *n) {
			if (!n->visibility)
				n->visibility = defaultVisibility;

			if (defaultThread) {
				if (Class *c = as<Class>(n)) {
					c->defaultThread(defaultThread);
				}
			}
		}

		void Content::update(NamedDecl *fn) {
			if (!fn->visibility)
				fn->visibility = defaultVisibility;

			if (!fn->thread)
				fn->thread = defaultThread;
		}

	}
}
