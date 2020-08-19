#include "stdafx.h"
#include "OffsetSource.h"
#include "OffsetReference.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace code {

	OffsetSource::OffsetSource() : data() {
		refs = new (this) WeakSet<OffsetReference>();
	}

	OffsetSource::OffsetSource(Offset offset) : data(offset) {
		refs = new (this) WeakSet<OffsetReference>();
	}

	void OffsetSource::set(Offset offset) {
		data = offset;
		update();
	}

	Offset OffsetSource::offset() const {
		if (stolenBy)
			return stolenBy->offset();
		else
			return data;
	}

	OffsetSource *OffsetSource::findActual() {
		if (stolenBy)
			return stolenBy->findActual();
		else
			return this;
	}

	void OffsetSource::update() {
		WeakSet<OffsetReference>::Iter i = refs->iter();
		while (OffsetReference *ref = i.next())
			ref->onMoved(data);
	}

	void OffsetSource::steal(OffsetSource *from) {
		WeakSet<OffsetReference>::Iter i = from->refs->iter();
		while (OffsetReference *ref = i.next()) {
			refs->put(ref);
			ref->to = this;
			ref->onMoved(data);
		}

		from->refs->clear();

		// Make sure it gets its information from us!
		from->stolenBy = this;
	}

	void OffsetSource::toS(StrBuf *to) const {
		*to << title();
	}


	StrOffsetSource::StrOffsetSource(const wchar *title) : OffsetSource() {
		name = new (this) Str(title);
	}

	StrOffsetSource::StrOffsetSource(Str *title) : OffsetSource(), name(title) {}

	StrOffsetSource::StrOffsetSource(Str *title, Offset offset) : OffsetSource(offset), name(title) {}

	Str *StrOffsetSource::title() const {
		return name;
	}

}
