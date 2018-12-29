#include "stdafx.h"
#include "Variant.h"
#include "Runtime.h"
#include "Handle.h"
#include "StrBuf.h"

namespace storm {

	Variant::Variant() : data(null) {}

	Variant::Variant(const Variant &o) : data(null) {
		if (!o.data)
			return;

		const GcType *t = runtime::gcTypeOf(o.data);
		if (t->kind == GcType::tArray) {
			// Value.
			GcArray<Byte> *alloc = (GcArray<Byte> *)o.data;
			init(alloc->v, t->type);
		} else {
			// Class.
			data = o.data;
		}
	}

	Variant::Variant(RootObject *o) : data(o) {}

	Variant::Variant(const void *value, Type *type) {
		init(value, type);
	}

	void Variant::init(const void *value, Type *type) {
		const Handle &h = runtime::typeHandle(type);
		data = runtime::allocArray(h.engine(), h.gcArrayType, 1);

		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		h.safeCopy(alloc->v, value);
	}

	Variant::~Variant() {
		if (!data)
			return;

		const GcType *t = runtime::gcTypeOf(data);
		if (t->kind != GcType::tArray)
			return;

		GcArray<Byte> *alloc = (GcArray<Byte> *)data;
		const Handle &h = runtime::typeHandle(t->type);
		h.safeDestroy(alloc->v);
	}

	void Variant::deepCopy(CloneEnv *env) {
		if (!data)
			return;

		const GcType *t = runtime::gcTypeOf(data);
		if (t->kind == GcType::tArray) {
			// Value.
			const Handle &h = runtime::typeHandle(t->type);
			if (!h.deepCopyFn)
				return;

			GcArray<Byte> *alloc = (GcArray<Byte> *)data;
			(*h.deepCopyFn)(alloc->v, env);
		} else {
			// Class.
			data = runtime::cloneObjectEnv((RootObject *)data, env);
		}
	}

	Bool Variant::has(Type *type) const {
		if (!data)
			return false;

		const GcType *t = runtime::gcTypeOf(data);
		return runtime::isA(type, t->type);
	}

	Engine &Variant::engine() const {
		const GcType *t = runtime::gcTypeOf(data);
		return runtime::allocEngine((RootObject *)t->type);
	}

	StrBuf &operator <<(StrBuf &to, const Variant &v) {
		if (v.empty())
			return to << S("<empty>");

		const GcType *t = runtime::gcTypeOf(v.data);
		const Handle &h = runtime::typeHandle(t->type);
		if (t->kind == GcType::tArray) {
			GcArray<Byte> *alloc = (GcArray<Byte> *)v.data;
			(*h.toSFn)(alloc->v, &to);
		} else {
			(*h.toSFn)(&v.data, &to);
		}
		return to;
	}

	wostream &operator <<(wostream &to, const Variant &v) {
		if (v.empty())
			return to << L"<empty>";

		StrBuf *b = new (v.engine()) StrBuf();
		*b << v;
		return to << ::toS(b);
	}

}
