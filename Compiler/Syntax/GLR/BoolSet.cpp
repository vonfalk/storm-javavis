#include "stdafx.h"
#include "BoolSet.h"

namespace storm {
	namespace syntax {
		namespace glr {

			BoolSet::BoolSet() : data(null) {}

			Bool BoolSet::get(Nat id) {
				if (id >= count())
					return false;
				else
					return ((data->v[entry(id)] >> offset(id)) & Nat(0x1)) != 0;
			}

			void BoolSet::set(Nat id, Bool v) {
				if (id >= count())
					grow(id + 1);

				Nat mask = Nat(1) << offset(id);
				if (v)
					data->v[entry(id)] |= mask;
				else
					data->v[entry(id)] &= ~mask;
			}

			void BoolSet::clear() {
				if (data)
					memset(data->v, 0, sizeof(Nat)*data->count);
			}

			void BoolSet::grow(Nat count) {
				Nat cap = (count + bitsPerEntry - 1) / bitsPerEntry;
				cap = max(Nat(minSize), cap);
				cap = max(this->count(), cap);

				GcArray<Nat> *n = runtime::allocArray<Nat>(engine(), &natArrayType, cap);
				if (data)
					memcpy(n->v, data->v, sizeof(Nat)*data->count);
				data = n;
			}

		}
	}
}
