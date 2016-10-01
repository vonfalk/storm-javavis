#include "stdafx.h"
#include "RefSource.h"
#include "Reference.h"
#include "Core/Array.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace code {

	Content::Content() {}

	void Content::set(const void *addr, nat size) {
		lastAddress = addr;
		lastSize = size;
	}

	StaticContent::StaticContent(const void *addr) {
		set(addr, 0);
	}


	RefSource::RefSource(const wchar *title) : cont(null) {
		name = new (this) Str(title);
		refs = new (this) WeakSet<Reference>();
	}

	RefSource::RefSource(Str *title) : name(title), cont(null) {
		refs = new (this) WeakSet<Reference>();
	}

	RefSource::RefSource(Str *title, Content *content) : name(title), cont(content) {
		refs = new (this) WeakSet<Reference>();
	}

	void RefSource::set(Content *to) {
		// NOTE: do _not_ ignore updating if 'cont == to', as we rely on the always update behaviour
		// to force updates in storm::DynamicCode.
		cont = to;

		Array<Reference *> *r = new (this) Array<Reference *>();

		WeakSet<Reference>::Iter i = refs->iter();
		while (Reference *ref = i.next())
			r->push(ref);

		const void *addr = address();
		for (Nat i = 0; i < r->count(); i++)
			r->at(i)->moved(addr);
	}

	void RefSource::setPtr(const void *to) {
		set(new (this) StaticContent(to));
	}

	void RefSource::toS(StrBuf *to) const {
		*to << name;
	}

}
