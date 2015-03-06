#include "stdafx.h"
#include "FnParams.h"

namespace code {

	FnParams::FnParams() : params(new Param[initialCapacity]), size(0), capacity(initialCapacity) {}

	FnParams::FnParams(void *buffer) : params((Param *)buffer), size(0), capacity(0) {}

	FnParams::FnParams(const FnParams &o) : params(null), size(o.size) {
		capacity = max(o.capacity, size);
		params = new Param[capacity];
		for (nat i = 0; i < size; i++)
			params[i] = o.params[i];
	}

	FnParams &FnParams::operator =(const FnParams &o) {
		if (capacity)
			delete []params;

		size = o.size;
		capacity = max(o.capacity, size);

		params = new Param[capacity];
		for (nat i = 0; i < size; i++)
			params[i] = o.params[i];
		return *this;
	}

	FnParams::~FnParams() {
		if (capacity)
			delete []params;
	}

	void FnParams::resize(nat to) {
		Param *n = new Param[to];
		for (nat i = 0; i < size; i++)
			n[i] = params[i];

		swap(n, params);
		capacity = to;
		delete []n;
	}

	void FnParams::add(CopyFn copy, DestroyFn destroy, nat paramSize, const void *value) {
		if (capacity != 0 && capacity == size)
			resize(capacity * 2);

		Param p = { copy, destroy, paramSize, value };
		params[size++] = p;
	}

	void FnParams::addFirst(CopyFn copy, DestroyFn destroy, nat paramSize, const void *value) {
		if (capacity != 0 && capacity == size)
			resize(capacity * 2);

		for (nat i = size; i > 0; i--)
			params[i] = params[i - 1];
		size++;

		Param p = { copy, destroy, paramSize, value };
		params[0] = p;
	}

	// Machine specific:
#ifdef X86

	nat FnParams::totalSize() const {
		nat r = 0;
		for (nat i = 0; i < size; i++)
			r += roundUp(params[i].size, sizeof(void *));
		return r;
	}

	void FnParams::copy(void *to) const {
		byte *at = (byte *)to;
		for (nat i = 0; i < size; i++) {
			const Param &p = params[i];
			if (p.copy)
				(*p.copy)(p.value, at);
			else
				*((const void **)at) = p.value;
			at += roundUp(p.size, sizeof(void *));
		}
	}

	void FnParams::destroy(void *to) const {
		byte *at = (byte *)to;
		for (nat i = 0; i < size; i++) {
			const Param &p = params[i];
			if (p.destroy)
				(*p.destroy)(at);
			at += roundUp(p.size, sizeof(void *));
		}
	}

	static void doCallLarge(void *result, nat retSz, const void *fn, const FnParams *params) {
		nat sz = params->totalSize();
		__asm {
			sub esp, sz;

			mov ecx, params;
			mov eax, esp;
			push eax;
			call FnParams::copy;

			push result;
			call fn;
			add esp, 4;
			add esp, sz;
		}
	}

	static void doCall4(void *result, const void *fn, const FnParams *params) {
		nat sz = params->totalSize();
		__asm {
			sub esp, sz;

			mov ecx, params;
			mov eax, esp;
			push eax;
			call FnParams::copy;

			call fn;
			add esp, sz;

			mov ecx, result;
			mov [ecx], eax;
		}
	}

	static void doCall8(void *result, const void *fn, const FnParams *params) {
		nat sz = params->totalSize();
		__asm {
			sub esp, sz;

			mov ecx, params;
			mov eax, esp;
			push eax;
			call FnParams::copy;

			call fn;
			add esp, sz;

			mov ecx, result;
			mov [ecx], eax;
			mov [ecx+4], edx;
		}
	}

	void doVoidCall(const void *fn, const FnParams *params) {
		nat dummy;
		doCall4(&dummy, fn, params);
	}

	float doFloatCall(const void *fn, const FnParams *params) {
		return (float)doDoubleCall(fn, params);
	}

	double doDoubleCall(const void *fn, const FnParams *params) {
		nat sz = params->totalSize();
		__asm {
			sub esp, sz;

			mov ecx, params;
			mov eax, esp;
			push eax;
			call FnParams::copy;

			call fn;
			add esp, sz;
		}
		// we're fine! Output is already stored on the floating-point stack.
	}

	void doCall(void *result, const TypeInfo &info, const void *fn, const FnParams *params) {
		nat s = roundUp(info.size, sizeof(void *));

		if (info.plain() && info.kind == TypeInfo::user) {
			// Returned on stack!
			doCallLarge(result, s, fn, params);
			return;
		}

		if (s <= 4)
			doCall4(result, fn, params);
		else if (s <= 8)
			doCall8(result, fn, params);
		else
			doCallLarge(result, s, fn, params);
	}

#else
#error "Please implement the function calls for your architecture."
#endif

}
