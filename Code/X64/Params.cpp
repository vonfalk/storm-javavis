#include "stdafx.h"
#include "Params.h"

namespace code {
	namespace x64 {

		Param::Param() {
			clear();
		}

		Param::Param(Nat id, Primitive p) {
			set(id, p.size().size64(), p.offset().v64());
		}

		Param::Param(Nat id, Nat size, Nat offset) {
			set(id, size, offset);
		}

		void Param::clear() {
			data = 0xFFFFFFFF;
		}

		void Param::set(Nat id, Nat size, Nat offset) {
			data = size & 0xF;
			data |= (id & 0xFF) << 4;
			data |= (offset & 0xFFFFFF) << 12;
		}

		StrBuf &operator <<(StrBuf &to, Param p) {
			if (p.id() == 0xFF) {
				to << S("empty");
			} else {
				to << S("#") << p.id() << S("+") << p.offset() << S(",") << p.size();
			}
			return to;
		}

		static void clear(GcArray<Param> *array) {
			for (Nat i = array->filled; i < array->count; i++)
				array->v[i].clear();
		}

		Params::Params() {
			integer = runtime::allocArray<Param>(engine(), &natArrayType, 6);
			real = runtime::allocArray<Param>(engine(), &natArrayType, 8);
			integer->filled = 0;
			real->filled = 0;
			clear(integer);
			clear(real);
			stack = new (this) Array<Nat>();
		}

		void Params::add(Nat id, TypeDesc *type) {
			if (PrimitiveDesc *p = as<PrimitiveDesc>(type)) {
				add(id, p->v);
			} else if (ComplexDesc *c = as<ComplexDesc>(type)) {
				addDesc(id, c);
			} else if (SimpleDesc *s = as<SimpleDesc>(type)) {
				addDesc(id, s);
			} else {
				assert(false, L"Unknown type description found.");
			}
		}

		void Params::add(Nat id, Primitive p) {
			switch (p.kind()) {
			case Primitive::none:
				// Nothing to do!
				break;
			case Primitive::pointer:
			case Primitive::integer:
				tryAdd(integer, Param(id, p));
				break;
			case Primitive::real:
				tryAdd(real, Param(id, p));
				break;
			default:
				dbg_assert(false, L"Unknown primitive type!");
			}
		}

		void Params::addDesc(Nat id, ComplexDesc *type) {
			// The complex types are actually simple. We just add a pointer to our parameter list!
			tryAdd(integer, Param(id, 8, 0));
		}

		void Params::addDesc(Nat id, SimpleDesc *type) {
			// Here, we should check 'type' to see if we shall pass parts of it in registers.
			// It seems the algorithm works roughly as follows:
			// - Examine each 8-byte word of the struct. Pass it in an integer register
			//   if it only contains integers and pointers. Pass it in a real register
			//   if it only contains floating point numbers. Otherwise, pass everything in memory.
			// - If the struct is larger than 4 8-byte words, always pass it in memory.
			// - If the struct is larger than 2 8-byte words, and do not contain real registers,
			//   pass it in memory.
			TODO(L"Implement me!");
		}

		void Params::tryAdd(GcArray<Param> *to, Param p) {
			if (to->filled < to->count) {
				to->v[to->filled++] = p;
			} else {
				stack->push(p.id());
			}
		}

		static void put(StrBuf *to, GcArray<Param> *p, const wchar **names) {
			for (Nat i = 0; i < p->filled; i++) {
				*to << S("\n") << names[i] << S(":") << p->v[i];
			}
		}

		void Params::toS(StrBuf *to) const {
			*to << S("Parameters:");
			const wchar *i[] = { S("RDI"), S("RSI"), S("RDX"), S("RCX"), S("R8"), S("R9") };
			const wchar *e[] = { S("XMM0"), S("XMM1"), S("XMM2"), S("XMM3"), S("XMM4"), S("XMM5"), S("XMM6"), S("XMM7") };
			put(to, integer, i);
			put(to, real, e);

			if (stack->count() > 0) {
				*to << S("\nOn stack: ") << stack->at(0);
				for (Nat i = 1; i < stack->count(); i++) {
					*to << S(", ") << stack->at(i);
				}
			}
		}

	}
}
