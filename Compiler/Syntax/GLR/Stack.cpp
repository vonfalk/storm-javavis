#include "stdafx.h"
#include "Stack.h"
#include "Core/StrBuf.h"
#include "Utils/Bitwise.h"

namespace storm {
	namespace syntax {
		namespace glr {

			StackItem::StackItem(Nat state) : state(state), prev(null) {}

			StackItem::StackItem(Nat state, StackItem *prev) : state(state), prev(prev) {}

			Bool StackItem::insert(StackItem *insert) {
				StackItem **at = &morePrev;
				for (; *at; at = &(*at)->morePrev) {
					if (*at == insert)
						return false;
				}

				*at = insert;
				return true;
			}

			Bool StackItem::equals(Object *o) const {
				if (runtime::typeOf(this) != runtime::typeOf(o))
					return false;

				return state == ((StackItem *)o)->state;
			}

			Nat StackItem::hash() const {
				return state;
			}

			static void print(const StackItem *me, const StackItem *end, StrBuf *to) {
				bool space = false;
				for (const StackItem *i = me; i; i = i->prev) {
					if (i == end) {
						*to << L"...";
						break;
					}
					if (space)
						*to << L" ";
					space = true;
					*to << i->state;

					if (i->reduced) {
						Indent z(to);
						*to << L"\n(";
						print(i->reduced, i->prev, to);
						*to << L")\n";
						space = false;
					}
				}
			}

			void StackItem::toS(StrBuf *to) const {
				print(this, null, to);
			}


			/**
			 * Future stacks.
			 */

			FutureStacks::FutureStacks() {
				data = null;
			}

			MAYBE(Set<StackItem *> *) FutureStacks::top() {
				if (data)
					return data->v[first];
				else
					return null;
			}

			void FutureStacks::pop() {
				if (data)
					data->v[first] = null;
				first = wrap(first + 1);
			}

			void FutureStacks::put(Nat pos, StackItem *insert) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				Set<StackItem *> *&to = data->v[i];
				if (!to)
					to = new (this) Set<StackItem *>();


				StackItem *old = to->at(insert);
				if (old == insert) {
					// Insertion was performed. Nothing more to do.
				} else {
					// Append the current node as an alternative.
					old->insert(insert);
				}
			}

			void FutureStacks::grow(Nat cap) {
				cap = max(Nat(32), nextPowerOfTwo(cap));

				GcArray<Set<StackItem *> *> *n = runtime::allocArray<Set<StackItem *> *>(engine(), &pointerArrayType, cap);
				Nat c = count();
				for (Nat i = 0; i < c; i++) {
					n->v[i] = data->v[wrap(first + i)];
				}

				data = n;
				first = 0;
			}

		}
	}
}
