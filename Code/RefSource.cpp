#include "StdAfx.h"
#include "RefSource.h"

#include "Arena.h"

namespace code {

	RefSource::RefSource(Arena &arena, const String &title) : arena(arena), content(null) {
		referenceId = arena.refManager.addSource(this, title);
	}

	RefSource::~RefSource() {
		clear();
	}

	void RefSource::clear() {
		if (referenceId != RefManager::invalid)
			arena.refManager.removeSource(referenceId);
		referenceId = RefManager::invalid;
	}

	const String &RefSource::title() const {
		return arena.refManager.name(referenceId);
	}

	void RefSource::set(Content *n) {
		if (n == content)
			return;

		content = n;
		arena.refManager.setContent(referenceId, content);
	}

	void RefSource::setPtr(void *ptr) {
		if (StaticContent *sc = dynamic_cast<StaticContent *>(content)) {
			sc->set(ptr);
		} else {
			set(new StaticContent(arena, ptr));
		}
	}

	Content::Content(Arena &arena) : arena(arena), lastAddress(null), lastSize(0) {}

	Content::~Content() {
		arena.refManager.contentDestroyed(this);
	}

	void Content::set(void *ptr, nat size) {
		lastAddress = ptr;
		lastSize = size;
		arena.refManager.updateAddress(this);
	}

	StaticContent::StaticContent(Arena &arena, void *ptr) : Content(arena) {
		set(ptr);
	}

}
