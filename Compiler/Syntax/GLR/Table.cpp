#include "stdafx.h"
#include "Table.h"
#include "Exception.h"

namespace storm {
	namespace syntax {
		namespace glr {

			RuleInfo::RuleInfo() : productions(null) {}

			void RuleInfo::push(Engine &e, Nat id) {
				if (!productions)
					productions = new (e) Array<Nat>();
				productions->push(id);
			}


			/**
			 * Syntax.
			 */

			Syntax::Syntax() {
				rules = new (this) Map<Rule *, RuleInfo>();
				prods = new (this) Map<Production *, Nat>();
			}

			void Syntax::add(Rule *rule) {
				// This creates the rule data if it is not already there.
				rules->at(rule);
			}

			void Syntax::add(Production *type) {
				if (prods->has(type))
					return;

				// New production!
				Nat id = prods->count();
				prods->put(type, id);
				rules->at(type->rule()).push(engine(), id);
			}

			static Bool find(Nat item, Array<Nat> *in) {
				for (Nat i = 0; i < in->count(); i++)
					if (in->at(i) == item)
						return true;
				return false;
			}

			static Bool compare(Array<Nat> *a, Array<Nat> *b) {
				Nat aCount = a ? a->count() : 0;
				Nat bCount = b ? b->count() : 0;
				if (aCount != bCount)
					return false;

				for (Nat i = 0; i < aCount; i++) {
					if (!find(a->at(i), b))
						return false;
				}

				return true;
			}

			Bool Syntax::sameSyntax(Syntax *o) {
				for (Map<Rule *, RuleInfo>::Iter i = rules->begin(), end = rules->end(); i != end; i++) {
					RuleInfo ours = i.v();
					RuleInfo their = o->rules->get(i.k(), RuleInfo());
					if (!compare(ours.productions, their.productions))
						return false;
				}

				for (Map<Rule *, RuleInfo>::Iter i = o->rules->begin(), end = o->rules->end(); i != end; i++) {
					if (!rules->has(i.k())) {
						if (i.v().productions && i.v().productions->any())
							return false;
					}
				}

				return true;
			}


			/**
			 * Items.
			 */

			Item::Item(Syntax *world, ProductionIter iter) {
				id = world->prods->get(iter.production(), -1);
				pos = iter.position();
			}

			Nat Item::hash() const {
				return (id << 8) ^ pos;
			}

			/**
			 * Item set.
			 */

			ItemSet::ItemSet() {}

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

			void ItemSet::push(Engine &e, Item item) {
				// Find insertion position.
				Nat insert = 0;
				Nat c = count();
				while (insert < c && data->v[insert] < item)
					insert++;

				// Already present?
				if (insert < c && item == data->v[insert])
					return;

				// Insert and move the others.
				if (!data || data->filled == data->count) {
					// New allocation is needed.
					GcArray<Item> *alloc = runtime::allocArray<Item>(e, &itemType, c + grow);

					memcpy(alloc->v, data->v, sizeof(Item)*insert);
					alloc->v[insert] = item;
					memcpy(alloc->v + insert + 1, data->v + insert, sizeof(Item)*(c - insert));
					data = alloc;
				} else {
					memmove(data->v + insert + 1, data->v + insert, sizeof(Item)*(c - insert));
					data->v[insert] = item;
				}
			}

			void push(EnginePtr e, ItemSet &to, Item item) {
				to.push(e.v, item);
			}


			/**
			 * Shift action.
			 */

			ShiftAction::ShiftAction(Regex regex, Nat state) : regex(regex), state(state) {}


			/**
			 * State.
			 */

			State::State() {
				actions = new (this) Array<ShiftAction *>();
			}


			/**
			 * Full table.
			 */

			Table::Table(Syntax *syntax) : syntax(syntax) {
				lookup = new (this) Map<ItemSet, Nat>();
				states = new (this) Array<State *>();
			}

			Bool Table::empty() const {
				return lookup->empty()
					&& states->empty();
			}

		}
	}
}
