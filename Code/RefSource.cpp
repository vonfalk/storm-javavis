#include "StdAfx.h"
#include "RefSource.h"

#include "Arena.h"

namespace code {

	RefSource::RefSource(Arena &arena, const String &title) : arena(arena), title(title), lastAddress(null) {
		referenceId = arena.refManager.addSource(this);
	}

	RefSource::~RefSource() {
		arena.refManager.removeSource(referenceId);
	}

	void RefSource::set(void *address, nat size) {
		if (lastAddress != address) {
			lastAddress = address;
			arena.refManager.setAddress(referenceId, address, size);
		}
	}

}
