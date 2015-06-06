#include "stdafx.h"
#include "Value.h"

namespace storm {

	ValueData::ValueData() : type(null), ref(false) {}

	ValueData::ValueData(Type *t, bool ref) : type(t), ref(ref) {}

	void ValueData::output(wostream &to) const {
		if (type == null)
			to << L"void";
		else
			to << typeIdentifier(type);
		if (ref)
			to << "&";
	}

	bool ValueData::operator ==(const ValueData &o) const {
		// No point in having references to null...
		if (type == null)
			return o.type == null;
		return type == o.type && ref == o.ref;
	}

}
