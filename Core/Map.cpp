#include "stdafx.h"
#include "Map.h"
#include "StrBuf.h"
#include "Utils/Bitwise.h"
#include <iomanip>

namespace storm {

	const GcType MapBase::infoType = {
		GcType::tArray,
		null,
		null,
		sizeof(Info),
		0,
		{},
	};

	MapBase::MapBase(const Handle &k, const Handle &v) : keyT(k), valT(v) {}

	MapBase::MapBase(MapBase *o) : keyT(o->keyT), valT(o->valT) {
		size = o->size;
		lastFree = o->lastFree;
		info = copyArray(o->info);
		key = copyArray(o->key, info, keyT);
		val = copyArray(o->val, info, valT);
	}

	void MapBase::deepCopy(CloneEnv *env) {
		if (keyT.deepCopyFn) {
			for (size_t i = 0; i < capacity(); i++) {
				if (info->v[i].status != Info::free) {
					(*keyT.deepCopyFn)(keyPtr(i), env);
				}
			}
		}

		if (valT.deepCopyFn) {
			for (size_t i = 0; i < capacity(); i++) {
				if (info->v[i].status != Info::free) {
					(*valT.deepCopyFn)(valPtr(i), env);
				}
			}
		}
	}

	void MapBase::clear() {
		size = 0;
		info = null;
		key = null;
		val = null;
	}

	void MapBase::shrink() {
		if (size == 0) {
			clear();
			return;
		}

		nat to = minCapacity;
		while (size > to)
			to *= 2;

		rehash(to);
	}

	void MapBase::toS(StrBuf *to) const {
		*to << L"{";
		bool first = true;

		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status == Info::free)
				continue;

			if (!first) {
				*to << L", ";
				first = false;
			}

			(*keyT.toSFn)(keyPtr(i), to);
			*to << L" -> ";
			(*valT.toSFn)(valPtr(i), to);
		}

		*to << L"}";
	}

	void MapBase::putRaw(const void *key, const void *value) {
		nat hash = (*keyT.hashFn)(key);
		nat old = findSlot(key, hash);
		if (old == Info::free) {
			insert(key, value, hash);
		} else {
			valT.safeDestroy(valPtr(old));
			valT.safeCopy(valPtr(old), value);
		}
	}

	Bool MapBase::hasRaw(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		return findSlot(key, hash) != Info::free;
	}

	void *MapBase::getRaw(const void *key) {
		nat hash = (*keyT.hashFn)(key);
		nat slot = findSlot(key, hash);
		if (slot == Info::free) {
			StrBuf *buf = new (this) StrBuf();
			*buf << L"The key ";
			(*keyT.toSFn)(key, buf);
			*buf << L" is not in the map.";
			throw MapError(::toS(buf));
		}
		return valPtr(slot);
	}

	void *MapBase::getRaw(const void *key, const void *def) {
		nat hash = (*keyT.hashFn)(key);
		nat slot = findSlot(key, hash);

		if (slot == Info::free)
			slot = insert(key, def, hash);

		return valPtr(slot);
	}

	void *MapBase::atRaw(const void *key, CreateCtor fn) {
		nat hash = (*keyT.hashFn)(key);
		nat slot = findSlot(key, hash);

		if (slot == Info::free) {
			slot = insert(key, hash);
			(*fn)(valPtr(slot), engine());
		}

		return valPtr(slot);
	}

	void MapBase::removeRaw(const void *key) {
		// Will break 'primarySlot' otherwise.
		if (capacity() == 0)
			return;

		nat hash = (*keyT.hashFn)(key);
		nat slot = primarySlot(hash);

		// Not in the map?
		if (info->v[slot].status == Info::free)
			return;

		nat prev = Info::free;
		do {
			if (info->v[slot].hash == hash && (*keyT.equalFn)(key, keyPtr(slot))) {
				// Is the node we're about to delete inside a chain?
				if (prev != Info::free) {
					// Unlink us from the chain.
					info->v[prev].status = info->v[slot].status;
				}

				nat next = info->v[slot].status;

				// Destroy the node.
				info->v[slot].status = Info::free;
				keyT.safeDestroy(keyPtr(slot));
				valT.safeDestroy(valPtr(slot));

				if (prev == Info::free && next != Info::end) {
					// The removed node was in the primary slot, and we need to move the next one into our slot.
					keyT.safeCopy(keyPtr(slot), keyPtr(next));
					valT.safeCopy(valPtr(slot), valPtr(next));
					info->v[slot] = info->v[next];
					info->v[next].status = Info::free;
					keyT.safeDestroy(keyPtr(next));
					valT.safeDestroy(valPtr(next));
				}

				size--;
				return;
			}

			prev = slot;
			slot = info->v[slot].status;
		} while (slot != Info::end);
	}

	Nat MapBase::countCollisions() const {
		Nat c = 0;
		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status != Info::free && i == primarySlot(info->v[i].hash)) {
				for (nat at = i; info->v[i].status != Info::end; i++)
					c++;
			}
		}
		return c;
	}

	Nat MapBase::countMaxChain() const {
		Nat c = 0;
		for (nat i = 0; i < capacity(); i++) {
			if (info->v[i].status != Info::free && i == primarySlot(info->v[i].hash)) {
				Nat len = 1;
				for (nat at = i; info->v[i].status != Info::end; i++)
					len++;
				c = max(c, len);
			}
		}
		return c;
	}

	void MapBase::dbg_print() {
		std::wcout << L"Map contents:" << endl;
		for (nat i = 0; i < capacity(); i++) {
			std::wcout << std::setw(2) << i << L": ";
			if (info->v[i].status == Info::free) {
				std::wcout << L"free";
			} else if (info->v[i].status == Info::end) {
				std::wcout << toHex(info->v[i].hash) << L" end";
			} else {
				std::wcout << toHex(info->v[i].hash) << L" -> " << info->v[i].status;
			}

			if (info->v[i].status != Info::free) {
				std::wcout << "   ";
				StrBuf *b = new (this) StrBuf();
				(*keyT.toSFn)(keyPtr(i), b);
				std::wcout << b;
			}
			std::wcout << endl;
		}
	}

	void MapBase::alloc(nat cap) {
		assert(info == null);
		assert(key == null);
		assert(val == null);
		assert(isPowerOfTwo(cap));

		size = 0;
		info = runtime::allocArray<Info>(engine(), &infoType, cap);
		key = runtime::allocArray<byte>(engine(), keyT.gcArrayType, cap);
		val = runtime::allocArray<byte>(engine(), valT.gcArrayType, cap);

		for (nat i = 0; i < cap; i++)
			info->v[i].status = Info::free;
	}

	void MapBase::grow() {
		nat c = capacity();
		if (c == 0) {
			// Initial table size.
			alloc(minCapacity);
		} else if (c == size) {
			// Keep to a multiple of 2.
			rehash(c * 2);
		}
	}

	void MapBase::rehash(nat cap) {
		nat oldSize = size;

		GcArray<Info> *oldInfo = info; info = null;
		GcArray<byte> *oldKey = key; key = null;
		GcArray<byte> *oldVal = val; val = null;

		alloc(cap);

		// Anything to do?
		if (oldInfo == null)
			return;

		try {
			// Insert all elements once again.
			for (nat i = 0; i < oldInfo->count; i++) {
				if (oldInfo->v[i].status == Info::free)
					continue;

				insert(keyPtr(oldKey, i), valPtr(oldVal, i), oldInfo->v[i].hash);
			}

			// The Gc will destroy the old arrays and all elements in there later on.
		} catch (...) {
			clear();

			// Restore old state.
			swap(oldSize, size);
			swap(oldInfo, info);
			swap(oldKey, key);
			swap(oldVal, val);
			throw;
		}
	}

	nat MapBase::insert(const void *key, const void *val, nat hash) {
		nat into = insert(key, hash);
		valT.safeCopy(valPtr(into), val);
		return into;
	}

	nat MapBase::insert(const void *key, nat hash) {
		grow();

		Info insert = { Info::end, hash };
		nat into = primarySlot(hash);

		if (info->v[into].status != Info::free) {
			// Check if the contained element is in its primary position.
			nat from = primarySlot(info->v[into].hash);
			if (from == into) {
				// It is in its primary position. Attach ourselves to the chain.
				nat to = freeSlot();
				insert.status = info->v[into].status;
				info->v[into].status = to;
				into = to;
			} else {
				// It is not. Move it somewhere else.

				// Walk the list from the original position and find the node before the one we're about to move...
				while (info->v[from].status != into)
					from = info->v[from].status;

				// Redo linking.
				nat to = freeSlot();
				info->v[from].status = to;

				// Move the node itself.
				info->v[to] = info->v[into];
				keyT.safeCopy(keyPtr(to), keyPtr(into));
				valT.safeCopy(valPtr(to), valPtr(into));
				keyT.safeDestroy(keyPtr(into));
				valT.safeDestroy(valPtr(into));
				info->v[into].status = Info::free;
			}
		}

		assert(info->v[into].status == Info::free, L"Internal error. Trying to overwrite a slot!");
		info->v[into] = insert;
		keyT.safeCopy(keyPtr(into), key);
		size++;

		return into;
	}

	nat MapBase::findSlot(const void *key, nat hash) {
		// Otherwise, primarySlot won't work.
		if (capacity() == 0)
			return Info::free;

		nat slot = primarySlot(hash);
		if (info->v[slot].status == Info::free)
			return Info::free;

		do {
			if (info->v[slot].hash == hash && (*keyT.equalFn)(key, keyPtr(slot)))
				return slot;

			slot = info->v[slot].status;
		} while (slot != Info::end);

		return Info::free;
	}

	nat MapBase::primarySlot(nat hash) const {
		// We know that 'capacity' is a power of two, therefore the following is equivalent to:
		// return hash % capacity;
		return hash & (capacity() - 1);
	}

	nat MapBase::freeSlot() {
		while (info->v[lastFree].status != Info::free)
			// We know that 'capacity' is a power of two. Therefore, the following is equivalent to:
			// if (++lastFree >= capacity) lastFree = 0;
			lastFree = (lastFree + 1) & (capacity() - 1);

		return lastFree;
	}

	GcArray<MapBase::Info> *MapBase::copyArray(const GcArray<Info> *src) {
		GcArray<Info> *dest = runtime::allocArray<Info>(engine(), &infoType, src->count);
		memcpy(dest->v, src->v, src->count*sizeof(Info));
		return dest;
	}

	GcArray<byte> *MapBase::copyArray(const GcArray<byte> *src, const GcArray<Info> *info, const Handle &type) {
		GcArray<byte> *dest = runtime::allocArray<byte>(engine(), type.gcArrayType, src->count);

		if (type.copyFn) {
			const byte *srcNow = src->v;
			byte *dstNow = dest->v;
			for (nat i = 0; i < src->count; i++) {
				// Copy only if there are valid objects there.
				if (info->v[i].status != Info::free)
					(*type.copyFn)(dstNow, srcNow);
				srcNow += type.size;
				dstNow += type.size;
			}
		} else {
			memcpy(dest->v, src->v, type.size*src->count);
		}

		return dest;
	}

	MapBase::Iter::Iter() : info(null), key(null), val(null), pos(0) {}

	MapBase::Iter::Iter(MapBase *owner) : info(owner->info), key(owner->key), val(owner->val), pos(0) {
		// Find the first occupied position. This may place us at the end.
		while (!atEnd() && info->v[pos].status == Info::free)
			pos++;
	}

	bool MapBase::Iter::operator ==(const Iter &o) const {
		if (atEnd() && o.atEnd())
			return true;
		else
			return key == o.key && val == o.val && pos == o.pos;
	}

	bool MapBase::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	MapBase::Iter &MapBase::Iter::operator ++() {
		if (!atEnd())
			pos++;

		// Find the next occupied position.
		while (!atEnd() && info->v[pos].status == Info::free)
			pos++;

		return *this;
	}

	MapBase::Iter MapBase::Iter::operator ++(int) {
		Iter t(*this);
		++*this;
		return t;
	}

	void *MapBase::Iter::rawKey() const {
		size_t s = runtime::gcTypeOf(key)->stride;
		return key->v + pos*s;
	}

	void *MapBase::Iter::rawVal() const {
		size_t s = runtime::gcTypeOf(val)->stride;
		return val->v + pos*s;
	}

	MapBase::Iter &MapBase::Iter::preIncRaw() {
		return operator ++();
	}

	MapBase::Iter MapBase::Iter::postIncRaw() {
		return operator ++(0);
	}

	bool MapBase::Iter::atEnd() const {
		if (key)
			return pos == key->count;
		else
			return true;
	}

	MapBase::Iter MapBase::beginRaw() {
		return Iter(this);
	}

	MapBase::Iter MapBase::endRaw() {
		return Iter();
	}

}
