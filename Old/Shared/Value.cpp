#include "stdafx.h"
#include "Value.h"

namespace storm {

	ValueData::ValueData() : type(null), ref(false) {}

	ValueData::ValueData(Type *t, bool ref) : type(t), ref(ref) {}

	wostream &operator <<(wostream &to, const ValueData &o) {
		if (o.type == null)
			to << L"void";
		else
			to << typeIdentifier(o.type);
		if (o.ref)
			to << "&";
		return to;
	}

	bool ValueData::operator ==(const ValueData &o) const {
		// No point in having references to null...
		if (type == null)
			return o.type == null;
		return type == o.type && ref == o.ref;
	}

#ifdef VISUAL_STUDIO
	vector<ValueData> valDataList(nat count, ...) {
		vector<ValueData> r;
		r.reserve(count);

		va_list l;
		va_start(l, count);

		for (nat i = 0; i < count; i++) {
			r.push_back(va_arg(l, ValueData));
		}

		va_end(l);
		return r;
	}
#endif
}