#include "stdafx.h"
#include "CloneEnv.h"
#include "TObject.h"

namespace storm {

	CloneEnv::CloneEnv() {
		const Handle &h = StormInfo<TObject>::handle(engine());
		MapBase *base = new (this) MapBase(h, h);
		data = (Map<Object *, Object *> *)base;
	}

	Object *CloneEnv::cloned(Object *o) {
		// It is ok if we accidentally insert an additional 'null', that will be overwritten soon
		// anyway!
		return data->get(o, null);
	}

	void CloneEnv::cloned(Object *o, Object *to) {
		data->put(o, to);
	}

}
