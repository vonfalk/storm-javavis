#include "stdafx.h"
#include "Content.h"
#include "Exception.h"
#include "Type.h"
#include "Core/Str.h"

namespace storm {
	namespace bs {

		Content::Content() {
			types = new (this) Array<Type *>();
			functions = new (this) Array<FunctionDecl *>();
			threads = new (this) Array<NamedThread *>();
			// templates = new (this) Map<Str *, TemplateAdapter *>();
		}

		void Content::add(Type *t) {
			types->push(t);
		}

		void Content::add(FunctionDecl *t) {
			functions->push(t);
		}

		void Content::add(NamedThread *t) {
			threads->push(t);
		}

		void Content::add(Template *t) {
			// Str *k = new (this) Str(t->name);
			// if (templates->has(k)) {
			// 	TemplateAdapter *&found = templates->get(k);
			// 	found->add(t);
			// } else {
			// 	TemplateAdapter *adapter = new (this) TemplateAdapter(k);
			// 	adapter->add(t);
			// 	templates->put(k, adapter);
			// }
			TODO(L"Implement me!");
		}

		void Content::add(TObject *o) {
			if (Type *t = as<Type>(o))
				add(t);
			else if (FunctionDecl *fn = as<FunctionDecl>(o))
				add(fn);
			else if (NamedThread *nt = as<NamedThread>(o))
				add(nt);
			else if (Template *t = as<Template>(o))
				add(t);
			else
				throw InternalError(L"add for Content does not expect " + ::toS(runtime::typeOf(o)->identifier()));
		}

	}
}
