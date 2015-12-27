#include "stdafx.h"
#include "Array.h"

namespace storm {

	ArrayBase::ArrayBase(const Handle &type) : handle(type), size(0), capacity(0), data(null) {}

	ArrayBase::ArrayBase(Par<ArrayBase> o) : handle(o->handle), size(o->size), capacity(o->size) {
		data = new byte[o->size * handle.size];
		try {
			for (size = 0; size < o->size; size++) {
				size_t offset = size * handle.size;
				(*handle.create)(data + offset, o->data + offset);
			}
		} catch (...) {
			destroy(data, size);
			throw;
		}
	}

	ArrayBase::~ArrayBase() {
		destroy(data, size);
	}

	void ArrayBase::deepCopy(Par<CloneEnv> env) {
		if (!handle.deepCopy)
			return;

		for (nat i = 0; i < size; i++) {
			size_t offset = i * handle.size;
			(*handle.deepCopy)(data + offset, env.borrow());
		}
	}

	void ArrayBase::reserve(Nat size) {
		ensure(size);
	}

	void ArrayBase::clear() {
		destroy(data, size);
		data = null;
		size = 0;
		capacity = 0;
	}

	void ArrayBase::erase(Nat id) {
		assert(id < size);

		// Where is there a non-constructed element? Size == none.
		nat hole = size;

		try {
			if (handle.destroy) {
				(*handle.destroy)(data + id * handle.size);
				hole = id;
			}

			for (nat i = id + 1; i < size; i++) {
				size_t offset = i * handle.size;
				size_t prev = offset - handle.size;
				(*handle.create)(data + prev, data + offset);
				hole = size;
				if (handle.destroy) {
					(*handle.destroy)(data + offset);
					hole = id;
				}
			}

			size--;

		} catch (...) {
			// We may have one non-destroyed element now. If so, we destroy
			// all elements after that (destructors are safer to call than constructors).
			if (hole >= size)
				throw;

			if (handle.destroy) {
				for (nat i = hole + 1; i < size; i++)
					(*handle.destroy)(data + id * handle.size);
				size = hole;
			}
			throw;
		}
	}

	void ArrayBase::destroy(byte *data, nat elements) {
		if (data == null)
			return;

		if (handle.destroy) {
			size_t elemSize = handle.size;
			size_t to = elements * elemSize;
			for (nat i = 0; i < to; i += elemSize)
				(*handle.destroy)(data + i);
		}

		delete[] data;
	}

	void ArrayBase::ensure(nat n) {
		if (capacity >= n)
			return;

		nat growTo = max(max(nat(4), n), capacity * 2);
		byte *to = new byte[growTo * handle.size];
		nat copied = 0;

		try {
			for (copied = 0; copied < size; copied++) {
				size_t offset = copied * handle.size;
				(*handle.create)(to + offset, data + offset);
			}

			destroy(data, size);
			data = to;
			capacity = growTo;
		} catch (...) {
			destroy(to, copied);
			throw;
		}
	}


	Str *ArrayBase::toS() {
		Auto<StrBuf> r = CREATE(StrBuf, this);

		r->add(L"[");

		if (count() > 0)
			handle.output(ptr(0), r.borrow());

		for (nat i = 1; i < count(); i++) {
			r->add(L", ");
			handle.output(ptr(i), r.borrow());
		}

		r->add(L"]");

		return r->toS();
	}

	ArrayBase::Iter ArrayBase::beginRaw() {
		return Iter(this, 0);
	}

	ArrayBase::Iter ArrayBase::endRaw() {
		return Iter(this, count());
	}

	ArrayBase::Iter::Iter() : index(0) {}

	ArrayBase::Iter::Iter(Par<ArrayBase> owner, nat index) : owner(owner), index(index) {}

	bool ArrayBase::Iter::atEnd() const {
		return !owner || index >= owner->count();
	}

	bool ArrayBase::Iter::operator ==(const Iter &o) const {
		if (atEnd() != o.atEnd())
			return false;
		if (atEnd())
			return true;

		return owner.borrow() == o.owner.borrow() && index == o.index;
	}

	bool ArrayBase::Iter::operator !=(const Iter &o) const {
		return !(*this == o);
	}

	ArrayBase::Iter &ArrayBase::Iter::operator ++() {
		index++;
		return *this;
	}

	ArrayBase::Iter ArrayBase::Iter::operator ++(int) {
		Iter c(*this);
		index++;
		return c;
	}

	ArrayBase::Iter &ArrayBase::Iter::preIncRaw() {
		return ++(*this);
	}

	ArrayBase::Iter ArrayBase::Iter::postIncRaw() {
		return (*this)++;
	}

	void *ArrayBase::Iter::getRaw() const {
		if (atEnd())
			throw ArrayError(L"Trying to access an invalid iterator!");
		return owner->atRaw(index);
	}

	Nat ArrayBase::Iter::getIndex() const {
		return index;
	}

}
