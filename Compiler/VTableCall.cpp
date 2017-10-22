#include "stdafx.h"
#include "VTableCall.h"
#include "Code/Listing.h"
#include "Core/StrBuf.h"
#include "Engine.h"

namespace storm {

	VTableCalls::VTableCalls() {
		cpp = new (this) Array<code::RefSource *>();
		storm = new (this) Array<code::RefSource *>();
	}


	code::RefSource *VTableCalls::get(VTableSlot slot) {
		TODO(L"Implement VTable calls in an architecture-independent manner.");

		switch (slot.type) {
		case VTableSlot::tCpp:
			return getCpp(slot.offset);
		case VTableSlot::tStorm:
			return getStorm(slot.offset);
		default:
			assert(false, L"Unknown slot type.");
			return null;
		}
	}

	code::RefSource *VTableCalls::getCpp(Nat offset) {
		if (cpp->count() > offset)
			if (code::RefSource *r = cpp->at(offset))
				return r;

		while (cpp->count() <= offset)
			cpp->push(null);

		using namespace code;

		Listing *l = new (this) Listing();
		*l << mov(ptrA, ptrRel(ptrStack, Offset::sPtr));
		*l << mov(ptrA, ptrRel(ptrA, Offset()));
		*l << jmp(ptrRel(ptrA, Offset::sPtr * offset));

		Binary *b = new (this) Binary(engine().arena(), l);

		StrBuf *buf = new (this) StrBuf();
		*buf << L"vtable cpp:" << offset;
		RefSource *src = new (this) RefSource(buf->toS(), b);
		cpp->at(offset) = src;
		return src;
	}

	code::RefSource *VTableCalls::getStorm(Nat offset) {
		if (storm->count() > offset)
			if (code::RefSource *r = storm->at(offset))
				return r;

		while (storm->count() <= offset)
			storm->push(null);

		using namespace code;

		Listing *l = new (this) Listing();
		*l << mov(ptrA, ptrRel(ptrStack, Offset::sPtr));
		*l << mov(ptrA, ptrRel(ptrA, Offset()));
		*l << mov(ptrA, ptrRel(ptrA, -Offset::sPtr * vtable::extraOffset));
		*l << jmp(ptrRel(ptrA, Offset::sPtr * (offset + 2))); // 2 for the 2 size_t members in arrays.

		Binary *b = new (this) Binary(engine().arena(), l);

		StrBuf *buf = new (this) StrBuf();
		*buf << L"vtable storm:" << offset;
		RefSource *src = new (this) RefSource(buf->toS(), b);
		storm->at(offset) = src;
		return src;
	}

}
