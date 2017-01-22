#include "stdafx.h"
#include "RefManager.h"

#include "OS/Thread.h"

#include <limits>

namespace code {
	using util::Lock;


	RefManager::RefManager() : firstFreeIndex(0), shutdown(false) {}

	RefManager::~RefManager() {
		clear();
	}

	void RefManager::clear() {
		// Note: It is OK to not take the source lock here since clearing is expected to be done as a final
		// operation, alone.

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
		from->address = null;

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
		to->address = address;
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
		Lock::L z(sourceLock);

		// This will fail in the unlikely event that we have 2^32 elements in our
		// array, in which case we would already be out of memory.
		nat id = firstFreeIndex;
		while (sources.count(id) > 0)
			id = nextId(id);

		firstFreeIndex = nextId(id);
		return id;
	}

	nat RefManager::addSource(RefSource *source, const String &name) {
		Lock::L z(sourceLock);
		nat id = freeId();

		SourceInfo *src = new SourceInfo;
		src->id = id;
		src->lightRefs = 0;
		src->content = null;
		src->name = name;
		src->alive = true;

		sources.insert(make_pair(id, src));
		return id;
	}

	void RefManager::removeSource(nat id) {
		SourceInfo *source = this->source(id);
		if (!source)
			return;

		source->alive = false;
		cleanSource(id, source);
	}

	RefManager::SourceInfo *RefManager::source(nat id) const {
		Lock::L z(sourceLock);
		SourceMap::const_iterator i = sources.find(id);
		assert(i != sources.end(), L"ID not found!");
		if (i == sources.end())
			return null;
		return i->second;
	}

	RefManager::SourceInfo *RefManager::sourceUnsafe(nat id) const {
		Lock::L z(sourceLock);
		SourceMap::const_iterator i = sources.find(id);
		if (i == sources.end())
			return null;
		return i->second;
	}

	RefManager::ContentInfo *RefManager::content(const Content *src) const {
		ContentMap::const_iterator i = contents.find(const_cast<Content *>(src));
		assert(i != contents.end(), L"Content not found!");
		if (i == contents.end())
			return null;
		return i->second;
	}

	RefManager::ContentInfo *RefManager::contentUnsafe(const Content *src) const {
		ContentMap::const_iterator i = contents.find(const_cast<Content *>(src));
		if (i == contents.end())
			return null;
		return i->second;
	}

	void RefManager::cleanSource(nat id, SourceInfo *source) {
		Lock::L z(sourceLock);

		if (source->alive)
			return;

		if (atomicRead(source->lightRefs) > 0)
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
			ReachableSet cycle;
			if (liveReachable(cycle, source)) {
				// We are reachable by a live RefSource.
				return;
			}

			// Break the cycle!
			source->refs.clear();
			// Erase has to be before 'detachContent' so that we inhibit further clean-events
			// for this source.
			sources.erase(id);
			detachContent(source);
			delete source;
		}
	}

	bool RefManager::liveReachable(ReachableSet &reachable, SourceInfo *at) const {
		Lock::L z(sourceLock);

		// PLN("At: " << at->name);
		// for (set<SourceInfo*>::iterator i = reachable.begin(); i != reachable.end(); i++)
		// 	PLN("=> " << (*i)->name);

		if (at->alive || atomicRead(at->lightRefs) > 0)
			return true;

		// Cycle found.
		if (reachable.count(at))
			return false;

		reachable.insert(at);

		for (RefSet::iterator i = at->refs.begin(), end = at->refs.end(); i != end; ++i) {
			ContentInfo *c = contentUnsafe(&i->content());
			if (!c)
				// If we get here, we're in the process of destroying a cycle somewhere.
				continue;

			for (SourceSet::iterator i = c->sources.begin(), end = c->sources.end(); i != end; ++i) {
				if (liveReachable(reachable, *i))
					return true;
			}
		}

		return false;
	}

	void RefManager::setContent(nat id, Content *content) {
		SourceInfo *source = this->source(id);
		if (!source)
			return;

		detachContent(source);
		attachContent(source, content);
	}

	void RefManager::updateAddress(Content *content) {
		ContentInfo *info = this->contentUnsafe(content);
		if (!info)
			return;

		broadcast(info, content->address());
	}

	void RefManager::contentDestroyed(Content *content) {
		assert(contents.count(content) == 0, L"Content was being destroyed after have been 'set' to something!");
	}

	nat RefManager::ownerOf(void *addr) const {
		assert(false, L"Not implemented yet!");
		return invalid;
	}

	void *RefManager::address(SourceInfo *from) {
		ContentInfo *c = from->content;
		if (c)
			return c->content->address();
		else
			return null;
	}

	void *RefManager::address(nat id) const {
		SourceInfo *s = source(id);
		if (s)
			return address(s);
		else
			return null;
	}

	const String &RefManager::name(nat id) const {
		return source(id)->name;
	}

	void RefManager::addLightRef(void *v) {
		SourceInfo *s = (SourceInfo *)v;
		if (s)
			atomicIncrement(s->lightRefs);
	}

	void RefManager::removeLightRef(void *v) {
		SourceInfo *s = (SourceInfo *)v;
		if (s) {
			assert(s->lightRefs > 0);
			atomicDecrement(s->lightRefs);
		}
	}

	nat RefManager::refId(void *v) {
		SourceInfo *s = (SourceInfo *)v;
		return s->id;
	}

	void *RefManager::lightRefPtr(nat id) {
		void *r = sourceUnsafe(id);
		addLightRef(r);
		return r;
	}

	void *RefManager::address(void *v) {
		SourceInfo *s = (SourceInfo *)v;
		return s->address;
	}

	const String &RefManager::name(void *v) {
		SourceInfo *s = (SourceInfo *)v;
		return s->name;
	}

	void *RefManager::addReference(Reference *r, nat id) {
		SourceInfo *src = source(id);
		assert(src);
		src->refs.insert(r);
		return address(src);
	}

	void RefManager::removeReference(Reference *r, nat id) {
		SourceInfo *src = sourceUnsafe(id);
		if (src) {
			src->refs.erase(r);
			cleanSource(id, src);
		}
	}

}
