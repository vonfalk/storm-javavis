#include "stdafx.h"
#include "Content.h"
#include "RefSource.h"

namespace code {

	Content::Content() {}

	const void *Content::address() const {
		return lastAddress;
	}

	nat Content::size() const {
		return lastSize;
	}

	void Content::set(const void *addr, Nat size) {
		lastAddress = addr;
		lastSize = size;

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

	RefSource *Content::stolenBy() const {
		return null;
	}

	StaticContent::StaticContent(const void *addr) {
		set(addr, 0);
	}


}
