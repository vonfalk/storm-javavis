#include "stdafx.h"
#include "VTable.h"

#define VTABLE_WARN_SIZE 100

namespace code {

#if defined(VS) || defined(GCC)
	void *vtableOf(void *object) {
		void **o = (void **)object;
		return *o;
	}

	void setVtable(void *object, void *vtable) {
		void **o = (void **)object;
		*o = vtable;
	}
#endif

	/**
	 * Helpers for handling VTables, these contain most of the platform-specifics.
	 */

#if defined(VS) && VS == 2008

	// Is the address readable?
	static bool readable(void *addr) {
		if (addr == null)
			return false;

		MEMORY_BASIC_INFORMATION info;
		nat r = VirtualQuery(addr, &info, sizeof(info));
		if (r != sizeof(info))
			return false;

		if (info.State == MEM_FREE)
			return false;

		if (info.Protect == 0)
			return false;
		if (info.Protect & PAGE_NOACCESS)
			return false;
		if (info.Protect & PAGE_GUARD)
			return false;
		if (info.Protect & PAGE_EXECUTE) // I think this is execute only.
			return false;

		return true;
	}

	static bool isNewVTable(void *addr) {
		nat *p = (nat *)addr;
		if (p[0] < 0xFF && p[1] < 0xFF)
			return true;
		return false;
	}

	static nat vtableSize(void *vtable) {
		void **table = (void **)vtable;
		assert(readable(table));

		// Find the size by looking at each address.
		nat size = 1;
		while (readable(table + size)) {
			if (!readable(table[size]))
				return size;

			if (isNewVTable(table[size]))
				return size;

			size++;
		}

		return size - 1;
	}

	static void **allocVTable(void *from, nat size) {
		void **f = ((void **)from) - 1;
		void **table = new void*[size + 1];
		for (nat i = 0; i < size + 1; i++)
			table[i] = f[i];
		return table + 1;
	}

	static void freeVTable(void *from) {
		void **f = ((void **)from) - 1;
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
			PLN(L"Machine code mismatch: Excepted 0x60 or 0xA0, got " << toHex(data[size]));
			assert(false);
			return 0;
		}
	}

#endif


	VTable::VTable(void *cppVTable) {
		size = vtableSize(cppVTable);
		if (size > VTABLE_WARN_SIZE) {
			PLN("Big vtable detected: " << size << L" bytes.");
			assert(false);
		}
		content = allocVTable(cppVTable, size);
	}

	VTable::~VTable() {
		freeVTable(content);
	}

	void VTable::setTo(void *object) {
		setVtable(object, content);
	}

	void VTable::set(void *fn, void *newFn) {
		nat id = functionOffset(fn);
		content[id] = newFn;
	}

}
