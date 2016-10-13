#pragma once
#include "Code/Reference.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Update pointers inside the ref object (we can not store pointers into object when using MPS).
	 */
	class MemberRef : public code::Reference {
		STORM_CLASS;
	public:
		// Update a pointer inside 'obj'.
		MemberRef(Object *obj, size_t offset, code::Ref ref, code::Content *from);

		// Notification of changed address.
		virtual void moved(const void *newAddr);

	private:
		// Object.
		Object *update;

		// Offset into 'update'.
		size_t offset;
	};

}
