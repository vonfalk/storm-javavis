#include "stdafx.h"
#include "MemberRef.h"

namespace code {

	MemberRef::MemberRef(Object *obj, size_t offset, code::Ref ref, code::Content *from) :
		code::Reference(ref, from), update(obj), offset(offset) {

		moved(address());
	}

	void MemberRef::moved(const void *newAddr) {
		void *obj = (byte *)update + offset;
		*(const void **)obj = newAddr;
	}

}
