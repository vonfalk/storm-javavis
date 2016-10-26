#include "stdafx.h"
#include "MemberRef.h"
#include "Utils/Memory.h"

namespace code {

	MemberRef::MemberRef(Object *obj, size_t offset, code::Ref ref, code::Content *from) :
		Reference(ref, from), update(obj), offset(offset) {

		moved(address());
	}

	MemberRef::MemberRef(GcArray<const void *> *arr, size_t entry, code::Ref ref, code::Content *from) :
		Reference(ref, from), update(arr), offset(OFFSET_OF(GcArray<const void *>, v[entry])) {

		moved(address());
	}

	void MemberRef::moved(const void *newAddr) {
		void *obj = (byte *)update + offset;
		*(const void **)obj = newAddr;
	}

}
