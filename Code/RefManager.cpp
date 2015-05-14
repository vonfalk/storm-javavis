#include "StdAfx.h"
#include "RefManager.h"

#include <limits>

namespace code {

	RefManager::RefManager() : firstFreeIndex(0), shutdown(false) {}

	RefManager::~RefManager() {
		clear();
	}

	void RefManager::clear() {
		// We need to clean out everything. All sources should be dead by now, which means
		// that everything is an island and can be cleared.
		vector<SourceInfo *> toDestroy;
		toDestroy.reserve(sources.size());

		// Tell ourselves that it is OK to not make graph searches.
		preShutdown();

		// Destroying things may alter the map itself.
		for (SourceMap::iterator i = sources.begin(), end = sources.end(); i != end; ++i)
			toDestroy.push_back(i->second);


		for (nat i = 0; i < toDestroy.size(); i++) {
			SourceInfo *source = toDestroy[i];
			assert(!source->alive, L"Source " + source->name + L" outlives the RefManager.");

			detachContent(source);
		}

		// We need to delay actually deleting the sources, otherwise destroy calls may try to access
		// dangling pointers.
		for (nat i = 0; i < toDestroy.size(); i++) {
			SourceInfo *source = toDestroy[i];
			delete source;
		}

		sources.clear();
		assert(contents.empty(), L"Some content still left!");
		contents.clear();
		shutdown = false;
	}

	void RefManager::preShutdown() {
		shutdown = true;
	}

	void RefManager::detachContent(SourceInfo *from) {
		ContentInfo *content = from->content;
		if (!content)
			return;

		content->sources.erase(from);
		from->content = null;

		if (content->sources.empty()) {
			// We're free to go!
			// Do we need any more checks here?
			contents.erase(content->content);
			delete content->content;
			delete content;
		}
	}

	void RefManager::attachContent(SourceInfo *to, Content *content) {
		ContentInfo *info;
		ContentMap::iterator i = contents.find(content);
		if (i == contents.end()) {
			info = new ContentInfo;
			info->content = content;
			contents.insert(make_pair(content, info));
		} else {
			info = i->second;
		}
		attachContent(to, info);
	}

	void RefManager::attachContent(SourceInfo *to, ContentInfo *content) {
		assert(to->content == null, L"Content needs to be detached first!");
		to->content = content;
		content->sources.insert(to);

		broadcast(to, content->content->address());
	}

	void RefManager::broadcast(SourceInfo *to, void *address) {
		for (RefSet::iterator i = to->refs.begin(), end = to->refs.end(); i != end; ++i) {
			Reference *ref = *i;
			ref->onAddressChanged(address);
		}
	}

	void RefManager::broadcast(ContentInfo *to, void *address) {
		for (SourceSet::iterator i = to->sources.begin(), end = to->sources.end(); i != end; ++i) {
			broadcast(*i, address);
		}
	}

	static nat nextId(nat c) {
		if (++c == RefManager::invalid)
			return 0;
		else
			return c;
	}

	nat RefManager::freeId() {
		// This will fail in the unlikely event that we have 2^32 elements in our
		// array, in which case we would already be out of memory.
		nat id = firstFreeIndex;
		while (sources.count(id) > 0)
			id = nextId(id);

		firstFreeIndex = nextId(id);
		return id;
	}

	nat RefManager::addSource(RefSource *source, const String &name) {
		nat id = freeId();

		SourceInfo *src = new SourceInfo;
		src->lightRefs = 0;
		src->content = null;
		src->name = name;
		src->alive = true;

		sources.insert(make_pair(id, src));
		return id;
	}

	void RefManager::removeSource(nat id) {
		SourceMap::iterator i = sources.find(id);
		assert(i != sources.end(), L"ID not found!");
		if (i == sources.end())
			return;

		SourceInfo *source = i->second;
		source->alive = false;

		cleanSource(id, source);
	}

	void RefManager::cleanSource(nat id, SourceInfo *source) {
		if (source->alive)
			return;

		// We do not want to do this (possibly expensive) cleanup if we're
		// terminating soon anyway!
		if (shutdown)
			return;

		// If we have no references to us, we can remove ourselves.
		if (source->refs.empty()) {
			detachContent(source);
			sources.erase(id);
			delete source;
		} else {
			// TODO: We may be a part of a dead cycle!
		}
	}

	void RefManager::setContent(nat id, Content *content) {
		SourceMap::iterator i = sources.find(id);
		assert(i != sources.end(), L"ID not found!");
		if (i == sources.end())
			return;

		SourceInfo *source = i->second;
		detachContent(source);
		attachContent(source, content);
	}

	void RefManager::updateAddress(Content *content) {
		ContentMap::iterator i = contents.find(content);
		if (i == contents.end())
			return;

		broadcast(i->second, content->address());
	}

	void RefManager::contentDestroyed(Content *content) {
		assert(contents.count(content) == 0, L"Content was being destroyed after have been 'set' to something!");
	}

	nat RefManager::ownerOf(void *addr) const {
		assert(false, L"Not implemented yet!");
	}

	void *RefManager::address(SourceInfo *from) {
		ContentInfo *c = from->content;
		if (c)
			return c->content->address();
		else
			return null;
	}

	void *RefManager::address(nat id) const {
		SourceMap::const_iterator i = sources.find(id);
		assert(i != sources.end());
		return address(i->second);
	}

	const String &RefManager::name(nat id) const {
		SourceMap::const_iterator i = sources.find(id);
		assert(i != sources.end());
		return i->second->name;
	}

	void RefManager::addLightRef(nat id) {
		assert(sources.count(id) == 1);
		atomicIncrement(sources[id]->lightRefs);
	}

	void RefManager::removeLightRef(nat id) {
		SourceMap::const_iterator i = sources.find(id);
		if (i != sources.end()) {
			atomicDecrement(i->second->lightRefs);
		}
	}

	void *RefManager::addReference(Reference *r, nat id) {
		assert(sources.count(id) == 1);
		SourceInfo *src = sources[id];
		src->refs.insert(r);
		return address(src);
	}

	void RefManager::removeReference(Reference *r, nat id) {
		SourceMap::const_iterator i = sources.find(id);
		if (i == sources.end())
			return;

		SourceInfo *src = i->second;
		src->refs.erase(r);
		cleanSource(id, src);
	}

}
