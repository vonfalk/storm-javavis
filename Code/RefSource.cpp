#include "StdAfx.h"
#include "RefSource.h"

#include "Arena.h"

namespace code {

	RefSource::RefSource(Arena &arena, const String &title) : arena(arena), title(title) {
		referenceId = arena.refManager.addSource(this);
	}

	RefSource::~RefSource() {
		arena.refManager.removeSource(referenceId);
	}

	void RefSource::set(void *address, nat size) {
		arena.refManager.setAddress(referenceId, address, size);
	}

}