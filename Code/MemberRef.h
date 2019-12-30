#pragma once
#include "Code/Reference.h"
#include "Core/GcArray.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Update pointers inside the ref object (we can not store pointers into object when using MPS).
	 *
	 * Note: constructors will immediatly update the address.
	 */
	class MemberRef : public code::Reference {
		STORM_CLASS;
	public:
		// Update a pointer inside 'obj'.
		MemberRef(RootObject *obj, size_t offset, code::Ref ref, code::Content *from);

		// Update an entry inside 'arr'.
		MemberRef(GcArray<const void *> *arr, size_t entry, code::Ref ref, code::Content *from);

		// Notification of changed address.
		virtual void moved(const void *newAddr);

		// Move the pointer we're to update.
		void move(RootObject *obj, size_t offset);
		void move(GcArray<const void *> *arr, size_t entry);

		// Disable the updating.
		void disable();

	private:
		// Object.
		UNKNOWN(PTR_GC) void *update;

		// Offset into 'update'.
		size_t offset;
	};

}
