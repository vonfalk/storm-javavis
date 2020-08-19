#include "stdafx.h"
#include "OffsetReference.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace code {

	OffsetReference::OffsetReference(OffsetSource *to, Content *inside) : owner(inside), to(to), delta() {
		if (this->to) {
			this->to = this->to->findActual();
			this->to->refs->put(this);
		}
	}

	OffsetReference::OffsetReference(OffsetSource *to, Offset offset, Content *inside) : owner(inside), to(to), delta(offset) {
		if (this->to) {
			this->to = this->to->findActual();
			this->to->refs->put(this);
		}
	}

	OffsetReference::OffsetReference(OffsetRef to, Content *inside) : owner(inside), to(to.to), delta(to.delta) {
		if (this->to) {
			this->to = this->to->findActual();
			this->to->refs->put(this);
		}
	}

	Offset OffsetReference::offset() const {
		if (to)
			return to->offset() + delta;
		else
			return delta;
	}

	void OffsetReference::moved(Offset offset) {}

	void OffsetReference::onMoved(Offset offset) {
		moved(offset + delta);
	}

	void OffsetReference::toS(StrBuf *t) const {
		if (to)
			*t << to;
		else
			*t << S("<null>");
		if (delta != Offset())
			*t << delta;
	}

	OffsetRef::OffsetRef() : to(null) {}

	OffsetRef::OffsetRef(OffsetSource *to) : to(to) {}

	OffsetRef::OffsetRef(OffsetReference *ref) : to(ref->to), delta(ref->delta) {}

	OffsetRef::OffsetRef(OffsetSource *to, Offset offset) : to(to), delta(offset) {}

	void OffsetRef::deepCopy(CloneEnv *env) {}

	Offset OffsetRef::offset() const {
		if (to)
			return to->offset() + delta;
		else
			return delta;
	}

	wostream &operator <<(wostream &to, const OffsetRef &r) {
		if (r.to)
			to << r.to->title()->c_str();
		else
			to << L"<null>";
		if (r.delta != Offset())
			to << r.delta;
		return to;
	}

	StrBuf &operator <<(StrBuf &to, OffsetRef r) {
		if (r.to)
			to << r.to->title();
		else
			to << S("<null>");
		if (r.delta != Offset())
			to << r.delta;
		return to;
	}


}
