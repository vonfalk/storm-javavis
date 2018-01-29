#include "stdafx.h"
#include "HandleWrap.h"

namespace storm {

	/**
	 * Handle value.
	 */

	HandleValue::HandleValue(const HandleRef &o) : type(o.type) {
		// TODO: Cache these allocations!
		data = runtime::allocArray<byte>(type.engine(), type.gcArrayType, 1);
		data->filled = 1;
		type.safeCopy(data->v, o.data);
	}

	HandleValue::HandleValue(const HandleValue &o) : type(o.type) {
		PLN(L"Alloc2");
		// TODO: Cache these allocations!
		data = runtime::allocArray<byte>(type.engine(), type.gcArrayType, 1);
		data->filled = 1;
		type.safeCopy(data->v, o.data->v);
	}

	HandleValue &HandleValue::operator =(const HandleValue &o) {
		assert(&o.type == &type, L"Can not mix different types using HandleValue.");
		if (&o == this)
			return *this;

		type.safeDestroy(data->v);
		type.safeCopy(data->v, o.data->v);

		return *this;
	}

	HandleValue &HandleValue::operator =(const HandleRef &o) {
		assert(&o.type == &type, L"Can not mix different types using HandleValue.");

		type.safeDestroy(data->v);
		type.safeCopy(data->v, o.data);

		return *this;
	}

	HandleValue::~HandleValue() {
		type.safeDestroy(data->v);
	}

	bool HandleValue::operator <(const HandleValue &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleValue.");
		assert(type.lessFn, L"No < for HandleValue!");
		return (*type.lessFn)(data->v, o.data->v);
	}

	bool HandleValue::operator <(const HandleRef &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleValue.");
		assert(type.lessFn, L"No < for HandleValue!");
		return (*type.lessFn)(data->v, o.data);
	}

	bool HandleValue::operator ==(const HandleValue &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleValue.");
		assert(type.hasEqual(), L"No == for HandleValue!");
		return type.equal(data->v, o.data->v);
	}

	bool HandleValue::operator ==(const HandleRef &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleValue.");
		assert(type.hasEqual(), L"No == for HandleValue!");
		return type.equal(data->v, o.data);
	}


	/**
	 * Handle reference.
	 */


	HandleRef::HandleRef(const Handle &type, byte *data) : type(type), data(data) {}

	HandleRef::HandleRef(const HandleValue &o) : type(o.type), data(o.data->v) {}

	HandleRef &HandleRef::operator =(const HandleRef &o) {
		assert(&o.type == &type, L"Can not mix different types using HandleRef.");
		if (&o == this)
			return *this;

		type.safeDestroy(data);
		type.safeCopy(data, o.data);

		return *this;
	}

	HandleRef &HandleRef::operator =(const HandleValue &o) {
		assert(&o.type == &type, L"Can not mix different types using HandleRef.");

		type.safeDestroy(data);
		type.safeCopy(data, o.data->v);

		return *this;
	}

	bool HandleRef::operator <(const HandleRef &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleRef.");
		assert(type.lessFn, L"No < for HandleRef!");
		return (*type.lessFn)(data, o.data);
	}

	bool HandleRef::operator <(const HandleValue &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleRef.");
		assert(type.lessFn, L"No < for HandleRef!");
		return (*type.lessFn)(data, o.data->v);
	}

	bool HandleRef::operator ==(const HandleRef &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleRef.");
		assert(type.hasEqual(), L"No == for HandleRef!");
		return type.equal(data, o.data);
	}

	bool HandleRef::operator ==(const HandleValue &o) const {
		assert(&o.type == &type, L"Can not mix different types using HandleRef.");
		assert(type.hasEqual(), L"No == for HandleRef!");
		return type.equal(data, o.data->v);
	}


	/**
	 * Iterator.
	 */

	HandleIter::HandleIter(const Handle &type, GcArray<byte> *data, size_t element) :
		type(&type), data(&data->v[type.size * element]) {}

	HandleIter &HandleIter::operator ++() {
		data += type->size;
		return *this;
	}

	HandleIter HandleIter::operator ++(int) {
		HandleIter t = *this;
		t.data += type->size;
		return t;
	}

	HandleIter &HandleIter::operator --() {
		data -= type->size;
		return *this;
	}

	HandleIter HandleIter::operator --(int) {
		HandleIter t = *this;
		t.data -= type->size;
		return t;
	}

	HandleIter &HandleIter::operator +=(ptrdiff_t o) {
		data += type->size * o;
		return *this;
	}

	HandleIter &HandleIter::operator -=(ptrdiff_t o) {
		data -= type->size * o;
		return *this;
	}

	ptrdiff_t HandleIter::operator -(const HandleIter &o) const {
		return (data - o.data) / type->size;
	}

}
