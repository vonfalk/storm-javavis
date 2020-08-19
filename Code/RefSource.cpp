#include "stdafx.h"
#include "RefSource.h"
#include "Reference.h"
#include "Exception.h"
#include "DelegatedRef.h"
#include "Core/Array.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace code {

	RefSource::RefSource() : cont(null) {
		refs = new (this) WeakSet<Reference>();
	}

	RefSource::RefSource(Content *content) : cont(content) {
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

	void RefSource::clear() {
		if (cont)
			cont->owner = null;
		cont = null;
		update();
	}

	void RefSource::update() {
		const void *addr = address();
		WeakSet<Reference>::Iter i = refs->iter();
		while (Reference *ref = i.next())
			ref->moved(addr);
	}

	RefSource *RefSource::findActual() {
		if (cont)
			if (RefSource *r = cont->stolenBy())
				return r->findActual();
		return this;
	}

	void RefSource::setPtr(const void *to) {
		set(new (this) StaticContent(to));
	}

	void RefSource::steal(RefSource *from) {
		const void *addr = address();
		WeakSet<Reference>::Iter i = from->refs->iter();
		while (Reference *ref = i.next()) {
			refs->put(ref);
			ref->to = this;
			ref->moved(addr);
		}

		from->refs->clear();

		// Keep 'from' updated while it is alive. Otherwise any 'Ref' instances will not be updated.
		from->set(new (this) StolenContent(this));
	}

	void RefSource::toS(StrBuf *to) const {
		*to << title();
	}


	/**
	 * NameRefSource.
	 */

	StrRefSource::StrRefSource(const wchar *title) : RefSource() {
		name = new (this) Str(title);
	}

	StrRefSource::StrRefSource(Str *title) : RefSource(), name(title) {}

	StrRefSource::StrRefSource(Str *title, Content *content) : RefSource(content), name(title) {}

	Str *StrRefSource::title() const {
		return name;
	}


}
