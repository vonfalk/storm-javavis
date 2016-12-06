#include "stdafx.h"
#include "VTableCpp.h"
#include "Utils/Platform.h"
#include "Utils/Memory.h"
#include "Core/GcType.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"

namespace storm {

	VTableCpp *VTableCpp::wrap(Engine &e, const void *vtable) {
		return new (e) VTableCpp(vtable, vtable::count(vtable), false);
	}

	VTableCpp *VTableCpp::wrap(Engine &e, const void *vtable, nat count) {
		return new (e) VTableCpp(vtable, count, false);
	}

	VTableCpp *VTableCpp::copy(Engine &e, const void *vtable) {
		return new (e) VTableCpp(vtable, vtable::count(vtable), true);
	}

	VTableCpp *VTableCpp::copy(Engine &e, const void *vtable, nat count) {
		return new (e) VTableCpp(vtable, count, true);
	}

	VTableCpp *VTableCpp::copy(Engine &e, const VTableCpp *src) {
		return new (e) VTableCpp(src->table(), src->count(), true);
	}

	VTableCpp::VTableCpp(const void *vtable, nat count, bool copy) {
		init(vtable, count, copy);
	}

	void VTableCpp::init(const void *vtable, nat count, bool copy) {
		refs = null;
		data = null;
		tabSize = count;
		raw = null;

		if (copy) {
			data = runtime::allocArray<const void *>(engine(), &pointerArrayType, count + vtable::extraOffset);
			data->filled = 0;

			const void ** src = (const void **)vtable - vtable::extraOffset;
			for (nat i = 1; i < count + vtable::extraOffset; i++) {
				data->v[i] = src[i];
			}
		} else {
			raw = (const void **)vtable;
		}
	}

	const void **VTableCpp::table() const {
		if (data)
			return &data->v[vtable::extraOffset];
		else
			return raw;
	}

	void VTableCpp::replace(const void *vtable) {
		replace(vtable, vtable::count(vtable));
	}

	void VTableCpp::replace(const VTableCpp *src) {
		replace(src->table(), src->count());
	}

	struct VTableSwitch {
		const void **from;
		const void **to;
	};

	static void vtableSwitch(RootObject *o, void *data) {
		VTableSwitch *s = (VTableSwitch *)data;

		if (vtable::from(o) == s->from)
			vtable::set(s->to, o);
	}

	void VTableCpp::replace(const void *vtable, nat count) {
		if (!data || count > this->count()) {
			// We need to replace the vtables on all live objects using the vtable.
			bool needWalk = used();
			VTableSwitch data;
			data.from = table();

			// Create the new vtable.
			init(vtable, count, true);

			data.to = table();

			// Don't walk the heap if we don't need to. That can be *very* expensive. In most cases
			// this happens right after we've created an object but before we have set the vtable to
			// an object, which means it is safe not to do the expensive heap walk.
			if (needWalk)
				engine().gc.walkObjects(&vtableSwitch, &data);
		} else {
			// We can copy, we know we can modify 'data'.
			const void *const* src = (const void *const*)vtable - vtable::extraOffset;
			for (nat i = 1; i < count + vtable::extraOffset; i++) {
				data->v[i] = src[i];
			}
		}
	}

	nat VTableCpp::count() const {
		return tabSize;
	}

	nat VTableCpp::findSlot(const void *fn) const {
		return vtable::find(table(), fn, count());
	}

	const void *VTableCpp::extra() const {
		if (data)
			return data->v[0];
		else
			return null;
	}

	void VTableCpp::extra(const void *to) {
		if (data)
			data->v[0] = to;
	}

	void VTableCpp::set(nat id, const void *to) {
		if (data)
			data->v[id + vtable::extraOffset] = to;
	}

	void VTableCpp::set(nat id, Function *fn) {
		assert(id < count());

		if (!refs)
			refs = runtime::allocArray<Function *>(engine(), &pointerArrayType, count());

		refs->v[id] = fn;
		set(id, fn->directRef().address());
	}

	Function *VTableCpp::get(nat id) const {
		assert(id < count());
		if (refs)
			return refs->v[id];
		else
			return null;
	}

	void VTableCpp::clear(nat id) {
		assert(id < count());

		if (refs) {
			refs->v[id] = null;
		}
		if (data)
			data->v[id + vtable::extraOffset] = null;
	}

	const void *VTableCpp::address() const {
		if (data)
			return data;
		else
			// Tag the pointer.
			return (const byte *)raw + 1;
	}

	nat VTableCpp::size() const {
		return count() * sizeof(const void *) + vtableAllocOffset();
	}

	bool VTableCpp::used() const {
		if (data)
			return data->filled != 0;
		else
			return true;
	}

	void VTableCpp::insert(RootObject *obj) {
		if (data)
			data->filled = 1;
		vtable::set(table(), obj);
	}

	void VTableCpp::insert(code::Listing *to, code::Var obj, code::Ref table) {
		using namespace code;
		Label lbl = to->label();

		// See if the pointer in 'table' is tagged with a 1 in the lower bit. If so, remove it and
		// skip the mark-phase since we're dealing with a raw vtable.
		*to << mov(ptrA, table);
		*to << mov(ptrC, ptrConst(1));
		*to << not(ptrC);
		*to << and(ptrC, ptrA);
		*to << cmp(ptrC, ptrA);
		*to << jmp(lbl, ifNotEqual);

		// Mark as used by setting 'filled' to 1.
		*to << mov(ptrRel(ptrA, Offset::sPtr), ptrConst(1));
		// The pointer is offset a bit.
		*to << add(ptrA, to->engine().ref(Engine::rVTableAllocOffset));

		*to << lbl;
		*to << mov(ptrC, obj);
		*to << mov(ptrRel(ptrC, Offset()), ptrA);
	}

	size_t VTableCpp::vtableAllocOffset() {
		return OFFSET_OF(GcArray<const void *>, v[vtable::extraOffset]);
	}

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
			} else if (data[size] == 0x20) {
				// zero byte version, always jumps to offset 0
				return 0;
			} else {
				PLN(L"Machine code mismatch: Expected 0x60 or 0xA0, got " << toHex(data[size]));
				DebugBreak();
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

		const void *from(const RootObject *object) {
			const void *const*o = (const void *const*)object;
			return o[0];
		}

		void set(const void *vtable, RootObject *to) {
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
