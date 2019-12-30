#include "stdafx.h"
#include "MemberRef.h"
#include "Utils/Memory.h"

namespace code {

	MemberRef::MemberRef(RootObject *obj, size_t offset, code::Ref ref, code::Content *from) :
		Reference(ref, from) {

		move(obj, offset);
	}

	MemberRef::MemberRef(GcArray<const void *> *arr, size_t entry, code::Ref ref, code::Content *from) :
		Reference(ref, from) {

		move(arr, entry);
	}

	void MemberRef::moved(const void *newAddr) {
		if (update) {
			void *obj = (byte *)update + offset;
			atomicWrite(*(size_t*)obj, (size_t)newAddr);
		}
	}

	void MemberRef::move(RootObject *obj, size_t offset) {
		update = obj;
		this->offset = offset;
		moved(address());
	}

	void MemberRef::move(GcArray<const void *> *arr, size_t entry) {
		update = arr;
		offset = OFFSET_OF(GcArray<const void *>, v[entry]);
		moved(address());
	}

	void MemberRef::disable() {
		update = null;
		offset = 0;
	}

}
