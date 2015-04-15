#include "stdafx.h"
#include "VTable.h"
#include "Memory.h"

#define VTABLE_WARN_SIZE 100

namespace code {

#if defined(VS) || defined(GCC)
	void *vtableOf(const void *object) {
		void **o = (void **)object;
		return *o;
	}

	void setVTable(void *object, void *vtable) {
		void **o = (void **)object;
		*o = vtable;
	}
#endif

	/**
	 * Helpers for handling VTables, these contain most of the platform-specifics.
	 */

#if defined(VS) && VS == 2008

	int VTable::extraOffset = -2;
	int VTable::dtorOffset = 0;

	static bool isNewVTable(void *addr) {
		nat *p = (nat *)addr;
		if (p[0] < 0xFF && p[1] < 0xFF)
			return true;
		return false;
	}

	nat vtableCount(void *vtable) {
		void **table = (void **)vtable;
		assert(readable(table));

		// Find the size by looking at each address.
		nat size = 1;
		while (readable(table + size)) {
			// For debugging, one of these are good.
			// flags(table + size);
			// flags(table[size]);

			if (!readable(table[size]))
				return size;

			if (isNewVTable(table[size]))
				return size;

			size++;
		}

		return size - 1;
	}

	static void **allocVTable(nat size) {
		void **table = new void*[size + 2];
		for (nat i = 0; i < size + 2; i++)
			table[i] = null;
		return table + 2;
	}

	static void copyVTable(void *from, void *to, nat size) {
		// Note: copies one byte extra before 'from' and 'to'.
		void **f = ((void **)from) - 1;
		void **t = ((void **)to) - 1;
		for (nat i = 0; i <= size; i++)
			t[i] = f[i];
	}

	static void freeVTable(void *from) {
		void **f = ((void **)from) - 2;
		delete[] f;
	}

	static void dumpCode(byte *ptr, nat size) {
		for (nat i = 0; i < size; i++) {
			if (i % 5 != 0)
				PNN(" ");
			PNN(toHex(ptr[i]));
			if ((i + 1) % 5 == 0)
				PLN("");
		}
		if (size % 5 != 0)
			PLN("");
	}

	// In debug mode in VS2008, the compiler links all functions to a global
	// table of "jmp X", to make incremental compilation easier. We need to
	// detect and follow this! Returns 'code' if it is not a jmp instruction.
	static void *followJmp(void *code) {
		byte *c = (byte *)code;

		if (c[0] == 0xE9) {
			// Found you!
			nat *rel = (nat *)(c + 1);
			return c + *rel + 5;
		} else {
			return code;
		}
	}

	static nat functionOffset(void *fn) {
		fn = followJmp(fn);

		// This is the code the compiler generates for a vtable call (VS2008)
		// We simply need to read the machine code and verify it, and then
		// extract the last byte/4 bytes and divide them by sizeof(void *)
		byte fnData[] = {
			// mov eax, DWORD PTR [esp+4]
			0x8B, 0x44, 0x24, 0x04,
			// mov eax, DWORD PTR [eax]
			0x8B, 0x00,
			// jmp DWORD PTR [eax+XX]
			0xFF, //0x60, 0xXX
			// or [eax+XXXXXXXX]
			// 0xFF, 0xA0, 0xXX, 0xXX, 0xXX, 0xXX
		};

		byte *data = (byte *)fn;
		nat size = ARRAY_SIZE(fnData);
		if (memcmp(fnData, data, size) != 0) {
			return VTable::invalid;
			// For debug:
			PLN("Mismatched machine code. Expected: ");
			dumpCode(fnData, size);
			PLN("Got:");
			dumpCode(data, size);
			assert(false);
		}

		if (data[size] == 0x60) {
			// one byte
			return nat(data[size + 1]) / sizeof(void *);
		} else if (data[size] == 0xA0) {
			// four bytes
			nat *ptr = (nat *)(data + size + 1);
			return *ptr / sizeof(void *);
		} else {
			// Debugging for developers.
			PLN(L"Machine code mismatch: Excepted 0x60 or 0xA0, got " << toHex(data[size]));
			assert(false);
			return VTable::invalid;
		}
	}

#endif

	VTable::VTable(void *cppVTable, nat count) {
		nat size = vtableCount(cppVTable);
		if (size > VTABLE_WARN_SIZE) {
			PLN("Big vtable detected: " << size << L" bytes.");
			assert(false);
		}

		if (count == 0)
			count = size;

		if (count < size) {
			PLN("Tried to allocate too small VTable: got " << count << ", needed " << size << ".");
			assert(false);
			count = size;
		}

		content = allocVTable(count);
		// Note: this may copy less than 'count' bytes!
		copyVTable(cppVTable, content, size);
		this->size = count;
	}

	VTable::VTable(const VTable &src) {
		size = src.size;
		content = allocVTable(size);
		copyVTable(src.content, content, size);
		extra(src.extra());
	}

	void VTable::replace(void *cppVTable) {
		nat newSize = vtableCount(cppVTable);
		assert(newSize <= size, "Tried to replace with a too big VTable!");
		copyVTable(cppVTable, content, newSize);
	}

	void VTable::replace(const VTable &src) {
		assert(src.size <= size, "Tried to replace with a too big VTable!");
		copyVTable(src.content, content, size);
	}

	VTable::~VTable() {
		freeVTable(content);
	}

	void VTable::setTo(void *object) {
		setVTable(object, content);
	}

	void VTable::set(nat slot, void *newFn) {
		assert(slot < size);
		content[slot] = newFn;
	}

	void VTable::set(void *fn, void *newFn) {
		nat slot = find(fn);
		assert(slot != invalid);
		set(slot, newFn);
	}

	void *VTable::get(nat slot) {
		assert(slot < size);
		return content[slot];
	}

	void VTable::setDtor(void *newFn) {
		set(dtorOffset, newFn);
	}

	nat VTable::find(void *fn) {
		return findSlot(fn, content, size);
	}

	void *VTable::extra() const {
		return content[extraOffset];
	}

	void VTable::extra(void *to) {
		content[extraOffset] = to;
	}

	nat findSlot(void *ptr, void *vtable, nat size) {
		nat slot = functionOffset(ptr);
		if (slot != VTable::invalid)
			return slot;

		if (size == 0)
			size = vtableCount(vtable);
		void **v = (void **)vtable;

		for (nat i = 0; i < size; i++)
			if (v[i] == ptr)
				return i;

		return VTable::invalid;
	}

	void *getSlot(void *ptr, nat slot) {
		void **p = (void **)ptr;
		return p[slot];
	}

	void *deVirtualize(void *ptr, void *vtable) {
		nat id = functionOffset(ptr);
		if (id == VTable::invalid)
			return null;
		void **v = (void **)vtable;
		return v[id];
	}

	void *vtableExtra(void *obj) {
		void **v = (void **)obj;
		return v[VTable::extraOffset];
	}

	void *vtableDtor(void *vtable) {
		void **v = (void **)vtable;
		return v[VTable::dtorOffset];
	}

}
