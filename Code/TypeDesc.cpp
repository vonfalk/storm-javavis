#include "stdafx.h"
#include "TypeDesc.h"
#include "Core/StrBuf.h"

namespace code {

	namespace primitive {
		const wchar *name(Kind k) {
			switch (k) {
			case none:
				return S("none");
			case pointer:
				return S("pointer");
			case integer:
				return S("integer");
			case real:
				return S("real");
			default:
				return S("<unknown>");
			}
		}
	}

	Primitive::Primitive() {
		// represents 'none:0@0'.
		dataA = 0;
		dataB = 0;
	}

	Primitive::Primitive(primitive::Kind kind, Size size, Offset offset) {
		dataA = Nat(kind) & 0x1;
		dataB = (Nat(kind) >> 1) & 0x1;

		dataA |= (size.size32() << 1) & 0xFE;
		dataB |= (size.size64() << 1) & 0xFE;

		dataA |= offset.v32() << 8;
		dataB |= offset.v64() << 8;
	}

	wostream &operator <<(wostream &to, const Primitive &p) {
		return to << primitive::name(p.kind()) << ":" << p.size() << L"@" << p.offset();
	}

	StrBuf &operator <<(StrBuf &to, Primitive p) {
		return to << primitive::name(p.kind()) << S(":") << p.size() << S("@") << p.offset();
	}

	Size TypeDesc::size() const {
		assert(false, L"Use a derived type.");
		return Size();
	}

	PrimitiveDesc::PrimitiveDesc(Primitive p) : v(p) {}

	void PrimitiveDesc::toS(StrBuf *to) const {
		*to << v;
	}

	ComplexDesc::ComplexDesc(Size size, const void *ctor, const void *dtor) : s(size), ctor(ctor), dtor(dtor) {}

	void ComplexDesc::toS(StrBuf *to) const {
		*to << S("complex:") << s;
	}

	static const GcType primitiveArray = {
		GcType::tArray,
		null,
		null,
		sizeof(Primitive),
		0,
		{},
	};

	SimpleDesc::SimpleDesc(Size size, Nat entries) : s(size) {
		v = runtime::allocArray<Primitive>(engine(), &primitiveArray, entries);
	}

	void SimpleDesc::deepCopy(CloneEnv *env) {
		GcArray<Primitive> *c = runtime::allocArray<Primitive>(engine(), &primitiveArray, v->count);
		memcpy(c->v, v->v, v->count * sizeof(Primitive));
		v = c;
	}

	void SimpleDesc::toS(StrBuf *to) const {
		*to << S("simple:") << s << S(":");
		for (Nat i = 0; i < v->count; i++) {
			*to << S("\n") << v->v[i];
		}
	}

}