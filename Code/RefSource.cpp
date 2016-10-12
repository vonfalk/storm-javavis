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

		if (owner)
			owner->update();
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
		if (cont != to) {
			assert(to->owner == null, L"Multiple owners of a single Content object.");

			if (cont)
				cont->owner = null;
			if (to)
				to->owner = this;
			cont = to;
		}

		update();
	}

	void RefSource::update() {
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
