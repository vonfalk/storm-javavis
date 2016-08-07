#include "stdafx.h"
#include "Array.h"
#include "StrBuf.h"

namespace storm {

	ArrayBase::ArrayBase(const Handle &type) : handle(type), data(null) {}

	ArrayBase::ArrayBase(ArrayBase *other) : handle(other->handle) {
		nat count = other->count();
		if (handle.copyFn) {
			ensure(count);

			for (nat i = 0; i < count; i++) {
				(*handle.copyFn)(ptr(i), other->ptr(i));
				// Remember the element was created, if we get an exception during copying.
				data->filled = i + 1;
			}
		} else {
			// Memcpy will do!
			ensure(count);
			memcpy(ptr(0), other->ptr(0), count * handle.size);
			data->filled = count;
		}
	}

	void ArrayBase::deepCopy(CloneEnv *env) {
		if (handle.deepCopyFn) {
			for (nat i = 0; i < data->filled; i++)
				(*handle.deepCopyFn)(ptr(i), env);
		}
	}

	void ArrayBase::ensure(Nat n) {
		Nat oldCap = data ? data->count : 0;
		if (oldCap >= n)
			return;

		// We need to grow 'data'. How much?
		Nat newCap = max(Nat(16), oldCap * 2);
		if (newCap < n)
			newCap = n;
		GcArray<byte> *newData = runtime::allocArray<byte>(engine(), handle.gcArrayType, newCap);

		// Move data.
		if (data) {
			memcpy(ptr(newData, 0), ptr(0), handle.size*oldCap);
			data->filled = 0;
			newData->filled = oldCap;
		}

		// Swap contents.
		data = newData;
	}

	void ArrayBase::reserve(Nat n) {
		ensure(n);
	}

	void ArrayBase::clear() {
		data = null;
	}

	void ArrayBase::toS(StrBuf *to) const {
		*to << L"[";
		if (count() > 1)
			(*handle.toSFn)(ptr(0), to);

		for (nat i = 1; i < count(); i++) {
			*to << L", ";
			(*handle.toSFn)(ptr(i), to);
		}
		*to << L"]";
	}

	void ArrayBase::pushRaw(const void *element) {
		Nat c = count();
		ensure(c + 1);

		if (handle.copyFn) {
			(*handle.copyFn)(ptr(c), element);
		} else {
			memcpy(ptr(c), element, handle.size);
		}
		data->filled = c + 1;
	}

	ArrayBase::Iter::Iter() : owner(0), index(0) {}

	ArrayBase::Iter::Iter(ArrayBase *owner, Nat index) : owner(owner), index(index) {}

	bool ArrayBase::Iter::operator ==(const Iter &o) const {
		if (atEnd() && o.atEnd())
			return true;
		else
			return index == o.index && owner == o.owner;
	}

	bool ArrayBase::Iter::atEnd() const {
		return owner == null
			|| owner->count() <= index;
	}

	bool ArrayBase::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	ArrayBase::Iter &ArrayBase::Iter::operator ++() {
		if (!atEnd())
			index++;
		return *this;
	}

	ArrayBase::Iter ArrayBase::Iter::operator ++(int) {
		Iter c = *this;
		++(*this);
		return c;
	}

	void *ArrayBase::Iter::getRaw() const {
		if (atEnd())
			throw ArrayError(L"Iterator pointing to the end of an array being dereferenced.");
		return owner->getRaw(index);
	}

	ArrayBase::Iter ArrayBase::Iter::preIncRaw() {
		return operator ++();
	}

	ArrayBase::Iter ArrayBase::Iter::postIncRaw() {
		return operator ++(0);
	}

}