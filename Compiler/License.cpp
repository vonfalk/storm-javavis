#include "stdafx.h"
#include "License.h"
#include "NameSet.h"
#include "Engine.h"
#include "Package.h"
#include "Core/StrBuf.h"

namespace storm {

	License::License(Str *id, Str *title, Str *body) : Named(id), title(title), body(body) {}

	void License::toS(StrBuf *to) const {
		const wchar *line = L"-----------------------------------";
		if (Named *p = as<Named>(parentLookup))
			*to << p->identifier() << L": ";
		*to << title << L"\n" << line << L"\n" << body << L"\n" << line;
	}

	static void licenses(Array<License *> *r, Named *root) {
		if (License *l = as<License>(root)) {
			r->push(l);
			return;
		}

		NameSet *search = as<NameSet>(root);
		if (!search)
			return;

		for (NameSet::Iter i = search->begin(), e = search->end(); i != e; ++i) {
			licenses(r, i.v());
		}
	}

	Array<License *> *licenses(Named *root) {
		Array<License *> *r = new (root) Array<License *>();
		licenses(r, root);
		return r;
	}

	Array<License *> *licenses(EnginePtr e) {
		return licenses(e.v.package());
	}

}
