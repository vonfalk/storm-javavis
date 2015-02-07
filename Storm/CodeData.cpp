#include "stdafx.h"
#include "CodeData.h"

namespace storm {

	void CodeData::add(code::Label lbl, const Fn<void, code::Listing &> &fn, Par<Object> keepAlive) {
		Item i = { lbl, fn, keepAlive };
		data.push_back(i);
	}

	void CodeData::output(code::Listing &to) const {
		for (nat i = 0; i < data.size(); i++) {
			const Item &d = data[i];
			to << d.label;
			d.generate(to);
		}
	}

}
