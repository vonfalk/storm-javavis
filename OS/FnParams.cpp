#include "stdafx.h"
#include "FnParams.h"
#include "Utils/Bitwise.h"

namespace os {

	FnParams::FnParams() : params(new Param[initialCapacity]), size(0), capacity(initialCapacity) {}

	FnParams::FnParams(void *buffer) : params((Param *)buffer), size(0), capacity(0) {}

	FnParams::FnParams(const FnParams &o) : params(null), size(o.size) {
		capacity = max(o.capacity, size);

		// This happens if 'o' is allocated using a separate buffer, and it contains zero parameters.
		if (capacity == 0)
			capacity = initialCapacity;

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

		if (capacity)
			delete []params;
		params = n;
		capacity = to;
	}

	void FnParams::add(CopyFn copy, DestroyFn destroy, nat paramSize, bool isFloat, const void *value) {
		if (capacity != 0 && capacity == size)
			resize(capacity * 2);

		Param p = { copy, destroy, value };
		p.isFloat = isFloat ? 1 : 0;
		p.size = paramSize;
		params[size++] = p;
	}

	void FnParams::addFirst(CopyFn copy, DestroyFn destroy, nat paramSize, bool isFloat, const void *value) {
		if (capacity != 0 && capacity == size)
			resize(capacity * 2);

		for (nat i = size; i > 0; i--)
			params[i] = params[i - 1];
		size++;

		Param p = { copy, destroy, value };
		p.isFloat = isFloat ? 1 : 0;
		p.size = paramSize;
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
				(*p.copy)(at, p.value);
			else
				memcpy(at, p.value, p.size);
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

#else
#error "Please implement stack layout for your architecture."
#endif

}
