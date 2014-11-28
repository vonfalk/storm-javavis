#include "StdAfx.h"
#include "RefManager.h"

#include <limits>

namespace code {

	const nat RefManager::invalid = std::numeric_limits<nat>::max();

	static nat nextId(nat c) {
		return (c == RefManager::invalid - 1) ? 0 : c + 1;
	}

	RefManager::RefManager() : firstFreeIndex(0) {}

	RefManager::~RefManager() {}

	nat RefManager::addSource(RefSource *source) {
		// This will fail in the unlikely event that we have 2^32 elements in our array, in which case we would already be out of memory.
		while (infoMap.count(firstFreeIndex) == 1)
			firstFreeIndex = nextId(firstFreeIndex);

		nat id = firstFreeIndex;
		firstFreeIndex = nextId(firstFreeIndex);

		Info *info = new Info;
		info->address = null;
		info->size = 0;
		info->lightCount = 0;
		info->source = source;

		infoMap[id] = info;
		addAddr(info->address, id);

		return id;
	}

	void RefManager::removeSource(nat id) {
		InfoMap::iterator i = infoMap.find(id);
		assert(i != infoMap.end());
		if (i == infoMap.end())
			return;

		Info *info = i->second;
		if (info->lightCount != 0 || !info->references.empty()) {
			PLN(info->source->getTitle() << L" still has live references!");
		}
		assert(info->lightCount == 0);
		assert(info->references.empty());
		infoMap.erase(id);
		removeAddr(info->address, id);
		delete info;
	}

	void RefManager::setAddress(nat id, void *address, nat size) {
		assert(infoMap.count(id));
		Info *info = infoMap[id];

		removeAddr(info->address, id);
		info->address = address;
		info->size = size;
		addAddr(info->address, id);

		typedef util::InlineSet<Reference>::iterator iter;
		iter end = info->references.end();
		for (iter i = info->references.begin(); i != end; ++i) {
			i->onAddressChanged(address);
		}
	}

	nat RefManager::ownerOf(void *addr) const {
		byte *val = (byte *)addr;
		AddrMap::const_iterator candidate = addresses.upper_bound(val+1);

		if (candidate == addresses.end())
			return invalid;

		nat id = candidate->second;
		Info *info = infoMap.find(id)->second;
		byte *from = (byte *)info->address;
		if (nat(val - from) < info->size) {
			return id;
		} else {
			return invalid;
		}
	}

	void *RefManager::address(nat id) const {
		InfoMap::const_iterator i = infoMap.find(id);
		assert(i != infoMap.end());
		return i->second->address;
	}

	const String &RefManager::name(nat id) const {
		return source(id)->getTitle();
	}

	RefSource *RefManager::source(nat id) const {
		InfoMap::const_iterator i = infoMap.find(id);
		assert(i != infoMap.end());
		return i->second->source;
	}

	void RefManager::addLightRef(nat id) {
		assert(infoMap.count(id) == 1);
		atomicIncrement(infoMap[id]->lightCount);
	}

	void RefManager::removeLightRef(nat id) {
		assert(infoMap.count(id) == 1);
		atomicDecrement(infoMap[id]->lightCount);
	}

	void *RefManager::addReference(Reference *r, nat id) {
		assert(infoMap.count(id) == 1);
		Info *info = infoMap[id];
		info->references.insert(r);
		return info->address;
	}

	void RefManager::removeReference(Reference *r, nat id) {
		assert(infoMap.count(id) == 1);
		Info *info = infoMap[id];
		info->references.erase(r);
	}

	void RefManager::addAddr(void *addr, nat id) {
		// Do not store "null".
		if (addr == null)
			return;

		addresses.insert(make_pair(addr, id));
	}

	void RefManager::removeAddr(void *addr, nat id) {
		// "null" is not stored.
		if (addr == null)
			return;

		AddrMap::iterator first = addresses.lower_bound(addr);
		AddrMap::iterator last = addresses.upper_bound(addr);

		nat removed = 0;

		while (first != last) {
			if (first->second == id) {
				first = addresses.erase(first);
				assert(++removed <= 1);
			} else {
				++first;
			}
		}

		assert(removed > 0);
	}
}
