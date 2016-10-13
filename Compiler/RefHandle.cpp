#include "stdafx.h"
#include "RefHandle.h"
#include "Utils/Memory.h"

namespace storm {

	RefHandle::RefHandle(code::Content *content) : content(content) {}

	void RefHandle::setCopyCtor(code::Ref ref) {
		copyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, copyFn), ref, content);
	}

	void RefHandle::setDestroy(code::Ref ref) {
		destroyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, destroyFn), ref, content);
	}

	void RefHandle::setDeepCopy(code::Ref ref) {
		deepCopyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, deepCopyFn), ref, content);
	}

	void RefHandle::setToS(code::Ref ref) {
		toSRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, toSFn), ref, content);
	}

	void RefHandle::setHash(code::Ref ref) {
		hashRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, hashFn), ref, content);
	}

	void RefHandle::setEqual(code::Ref ref) {
		equalRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, equalFn), ref, content);
	}

}
