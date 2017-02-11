#include "stdafx.h"
#include "Item.h"
#include "Exception.h"

namespace storm {
	namespace syntax {
		namespace glr {

			/**
			 * Item.
			 */

			Item::Item(Syntax *world, ProductionIter iter) {
				id = world->lookup->get(iter.production());
				pos = iter.position();
			}

			ProductionIter Item::iter(Syntax *syntax) const {
				Production *p = syntax->productions->at(id);
				return p->posIter(pos);
			}

			Nat Item::hash() const {
				return (id << 8) ^ pos;
			}

			Str *Item::toS(Syntax *syntax) const {
				StrBuf *to = new (syntax) StrBuf();
				*to << id << L": " << iter(syntax);
				return to->toS();
			}

			StrBuf &operator <<(StrBuf &to, Item item) {
				return to << L"(" << item.id << L", " << item.pos << L")";
			}


			/**
			 * Item set.
			 */

			ItemSet::ItemSet() : data(null) {}

			Item ItemSet::operator [](Nat id) const {
				if (id >= count())
					throw InternalError(L"Index out of bounds!");
				return at(id);
			}

			Bool ItemSet::operator ==(const ItemSet &o) const {
				Nat c = count();
				if (c != o.count())
					return false;

				for (Nat i = 0; i < c; i++)
					if (data->v[i] != o.data->v[i])
						return false;

				return true;
			}

			Bool ItemSet::operator !=(const ItemSet &o) const {
				return !(*this == o);
			}

			Bool ItemSet::has(Item i) const {
				Nat p = itemPos(i);
				if (p >= count())
					return false;
				return data->v[p] == i;
			}

			Nat ItemSet::hash() const {
				Nat r = 5381;
				Nat c = count();
				for (Nat i = 0; i < c; i++) {
					r = ((r << 5) + r) + data->v[i].hash();
				}
				return r;
			}

			static const GcType itemType = {
				GcType::tArray,
				null,
				null,
				sizeof(Item),
				0,
				{},
			};

			Nat ItemSet::itemPos(Item find) const {
				Nat from = 0;
				Nat to = count();
				while (from < to) {
					nat mid = (to - from) / 2 + from;

					if (data->v[mid] < find) {
						from = mid + 1;
					} else {
						to = mid;
					}
				}

				return from;
			}

			Bool ItemSet::push(Engine &e, Item item) {
				// Find insertion position.
				Nat c = count();
				Nat insert = itemPos(item);

				// Already present?
				if (insert < c && item == data->v[insert])
					return false;

				// Insert and move the others.
				if (!data || data->filled == data->count) {
					// New allocation is needed.
					GcArray<Item> *alloc = runtime::allocArray<Item>(e, &itemType, c + grow);

					alloc->v[insert] = item;
					if (data) {
						alloc->filled = data->filled;
						memcpy(alloc->v, data->v, sizeof(Item)*insert);
						memcpy(alloc->v + insert + 1, data->v + insert, sizeof(Item)*(c - insert));
					}
					data = alloc;
				} else {
					memmove(data->v + insert + 1, data->v + insert, sizeof(Item)*(c - insert));
					data->v[insert] = item;
				}

				data->filled++;
				return true;
			}

			Bool ItemSet::push(Syntax *syntax, ProductionIter iter) {
				if (!iter.valid())
					return false;

				return push(syntax->engine(), Item(syntax, iter));
			}

			static void expand(Engine &e, ItemSet &to, const Item &i, Syntax *syntax) {
				if (!to.push(e, i))
					return;

				ProductionIter iter = i.iter(syntax);
				RuleToken *rule = as<RuleToken>(iter.token());
				if (!rule)
					return;

				RuleInfo info = syntax->rules->get(rule->rule);
				for (Nat i = 0; i < info.count(); i++) {
					Production *p = syntax->productions->at(info[i]);

					ProductionIter a = p->firstA();
					if (a.valid())
						expand(e, to, Item(syntax, a), syntax);
					ProductionIter b = p->firstB();
					if (b.valid())
						expand(e, to, Item(syntax, b), syntax);
				}
			}

			ItemSet ItemSet::expand(Syntax *syntax) const {
				ItemSet result;
				Engine &e = syntax->engine();

				for (Nat i = 0; i < count(); i++) {
					glr::expand(e, result, at(i), syntax);
				}

				return result;
			}

			Str *ItemSet::toS(Syntax *syntax) const {
				StrBuf *to = new (syntax) StrBuf();
				*to << L"{\n";
				{
					Indent z(to);
					for (Nat i = 0; i < count(); i++) {
						*to << at(i).iter(syntax) << L"\n";
					}
				}
				*to << L"}";
				return to->toS();
			}

			Bool push(EnginePtr e, ItemSet &to, Item item) {
				return to.push(e.v, item);
			}

			StrBuf &operator <<(StrBuf &to, ItemSet s) {
				to << L"{\n";
				{
					Indent z(&to);
					for (Nat i = 0; i < s.count(); i++) {
						to << s.at(i) << L"\n";
					}
				}
				to << L"}";
				return to;
			}

		}
	}
}
