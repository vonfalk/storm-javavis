#include "stdafx.h"
#include "VTableCpp.h"
#include "Utils/Platform.h"
#include "Utils/Memory.h"

namespace storm {



	namespace vtable {

		const nat invalid = -1;

#if defined(VISUAL_STUDIO) && defined(X86)

		// Visual Studio stores something at offset -1, so we use -2!
		const nat extraOffset = 2;
		const nat dtorOffset = 0;

		static void dumpCode(const byte *ptr, nat size) {
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

		// In debug mode, Visual Studio links all functions to a global table of "jmp X" (probably
		// to make incremental linking easier). We need to detect and follow this! Returns 'code' if
		// is not a 'jmp' instruction.
		static const void *followJmp(const void *code) {
			const byte *c = (const byte *)code;

			if (c[0] == 0xE9) {
				// Found you!
				const nat *rel = (const nat *)(c + 1);
				return c + *rel + 5;
			} else {
				return code;
			}
		}

		nat fnSlot(const void *fn) {
			fn = followJmp(fn);

			// This is the code the compiler generates for a vtable call using cdecl calling
			// convention (VS2008). We simply need to read the machine code and verify it, and then
			// extract the last byte/4 bytes and divide them by sizeof(void *)
			static const byte fnData[] = {
				// mov eax, DWORD PTR [esp+4]
				0x8B, 0x44, 0x24, 0x04,
				// mov eax, DWORD PTR [eax]
				0x8B, 0x00,
				// jmp DWORD PTR [eax+XX]
				0xFF, //0x60, 0xXX
				// or [eax+XXXXXXXX]
				// 0xFF, 0xA0, 0xXX, 0xXX, 0xXX, 0xXX
			};

			const byte *data = (const byte *)fn;
			nat size = ARRAY_COUNT(fnData);
			if (memcmp(fnData, data, size) != 0) {
				return invalid;
				// For debugging:
				PLN(L"Mismatched machine code. Expected:");
				dumpCode(fnData, size);
				PLN(L"Got:");
				dumpCode(data, size);
				return invalid;
			}

			if (data[size] == 0x60) {
				// one byte version
				return nat(data[size + 1]) / sizeof(void *);
			} else if (data[size] == 0xA0) {
				// four byte version
				const nat *ptr = (const nat *)(data + size + 1);
				return *ptr / sizeof(void *);
			} else {
				PLN(L"Machine code mismatch: Expected 0x60 or 0xA0, got " << toHex(data[size]));
				return invalid;
			}
		}

		// Crude but useful heurustic to find the start of the next vtable.
		static bool isNewVTable(const void *addr) {
			const nat *p = (const nat *)addr;
			if (p[0] < 0xFF && p[1] < 0xFF)
				return true;
			return false;
		}

		nat count(const void *vtable) {
			const void *const* table = (const void *const*)vtable;
			assert(readable(table));

			// This is a table of pointers, so we can find the size by scanning until we find
			// something which does not look like a pointer.
			nat size = 1;
			while (readable(table + size)) {
				// For debugging, try:
				// memFlags(table + size);
				// memFlags(table[size]);

				// TODO: Check if the address is aligned?

				if (!readable(table[size]))
					return size;

				if (isNewVTable(table[size]))
					return size;

				size++;
			}

			return size - 1;
		}

#else
#error "I do not know how VTables work on your machine!"
#endif

#if defined(VISUAL_STUDIO) || defined(GCC)

		const void *from(const void *object) {
			const void *const*o = (const void *const*)object;
			return o[0];
		}

		void set(const void *vtable, void *to) {
			const void **o = (const void **)to;
			o[0] = vtable;
		}

#else
#error "I do not know how VTables work on your compiler!"
#endif

		const void *deVirtualize(const void *vtable, const void *fnPtr) {
			nat id = fnSlot(fnPtr);
			if (id == invalid)
				return null;
			const void *const*v = (const void *const*)vtable;
			return v[id];
		}

		nat find(const void *vtable, const void *fn) {
			return find(vtable, fn, 0);
		}

		nat find(const void *vtable, const void *fn, nat size) {
			nat slot = fnSlot(fn);
			if (slot != invalid)
				return slot;

			if (size == 0)
				size = count(vtable);

			const void *const*v = (const void *const*)vtable;
			nat result = invalid;
			for (nat i = 0; i < size; i++) {
				if (v[i] == fn) {
					if (result != invalid) {
						WARNING(L"Multiple entries with the same address in the VTable!");
						WARNING(L"Please use a non de-virtualized pointer instead!");
					}
					result = i;
				}
			}

			return result;
		}

		const void *slot(const void *vtable, nat slot) {
			const void *const*v = (const void *const*)vtable;
			return v[slot];
		}

	}
}
