#include "stdafx.h"
#include "Content.h"
#include "RefSource.h"

namespace code {

	Content::Content() {}

	const void *Content::address() const {
		if (lastAddress)
			return lastAddress;
		else
			return (const void *)lastOffset;
	}

	nat Content::size() const {
		return lastSize;
	}

	void Content::set(const void *addr, Nat size) {
		lastAddress = addr;
		lastOffset = 0;
		lastSize = size;

		if (owner)
			owner->update();
	}

	void Content::setOffset(Nat offset) {
		lastAddress = null;
		lastOffset = offset;
		lastSize = 0;

		if (owner)
			owner->update();
	}

	Str *Content::ownerName() const {
		RefSource *o = (RefSource *)atomicRead((void *&)owner);
		if (o)
			return o->title();
		else
			return null;
	}

	StaticContent::StaticContent(const void *addr) {
		set(addr, 0);
	}

	StaticContent::StaticContent(Nat offset) {
		setOffset(offset);
	}


}
