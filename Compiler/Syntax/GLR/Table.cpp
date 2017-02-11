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

			void push(EnginePtr e, RuleInfo &to, Nat id) {
				to.push(e.v, id);
			}

			/**
			 * Syntax.
			 */

			Syntax::Syntax() {
				rules = new (this) Map<Rule *, RuleInfo>();
				lookup = new (this) Map<Production *, Nat>();
				productions = new (this) Array<Production *>();
			}

			void Syntax::add(Rule *rule) {
				// This creates the rule data if it is not already there.
				rules->at(rule);
			}

			void Syntax::add(Production *p) {
				if (lookup->has(p))
					return;

				// New production!
				Nat id = productions->count();
				productions->push(p);
				lookup->put(p, id);
				rules->at(p->rule()).push(engine(), id);
			}

			Bool Syntax::sameSyntax(Syntax *o) {
				if (productions->count() != o->productions->count())
					return false;

				for (Nat i = 0; i < productions->count(); i++) {
					if (!o->lookup->has(productions->at(i)))
						return false;
				}

				return true;
			}


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


			/**
			 * Shift action.
			 */

			ShiftAction::ShiftAction(Regex regex, Nat state) : regex(regex), state(state) {}

			void ShiftAction::toS(StrBuf *to) const {
				*to << L"\"" << regex << L"\" -> " << state;
			}

			/**
			 * State.
			 */

			State::State(ItemSet items) : items(items), actions(null), rules(null), reduce(null) {}

			void State::toS(StrBuf *to) const {
				toS(to, null);
			}

			void State::toS(StrBuf *to, Syntax *syntax) const {
				if (actions && rules) {
					*to << L"Actions:\n";
					Indent z(to);
					for (Nat i = 0; i < actions->count(); i++) {
						*to << L"shift ";
						actions->at(i)->toS(to);
						*to << L"\n";
					}

					for (Map<Rule *, Nat>::Iter i = rules->begin(), end = rules->end(); i != end; ++i) {
						*to << L"goto " << i.k()->identifier() << L" -> " << i.v() << L"\n";
					}

					for (Nat i = 0; i < reduce->count(); i++) {
						*to << L"reduce " << reduce->at(i) << L"\n";
					}
				} else {
					*to << L"<to be computed>\n";
				}

				*to << L"Items:\n";
				for (Nat i = 0; i < items.count(); i++) {
					if (syntax)
						*to << items[i].toS(syntax);
					else
						*to << items[i];
					*to << L"\n";
				}
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

			Nat Table::state(ItemSet s) {
				Map<ItemSet, Nat>::Iter found = lookup->find(s);
				if (found != lookup->end())
					return found.v();

				Nat id = states->count();
				states->push(new (this) State(s));
				return id;
			}

			State *Table::state(Nat id) {
				State *s = states->at(id);
				if (!s->actions)
					fill(s);
				return s;
			}

			void Table::fill(State *state) {
				state->actions = new (this) Array<ShiftAction *>();
				state->rules = new (this) Map<Rule *, Nat>();
				state->reduce = new (this) Array<Nat>();

				// TODO: We might want to optimize this in the future. Currently we are using O(n^2)
				// time, but this can be done in O(n) time using a hash map, but that is probably
				// overkill for < 40 elements or so.

				ItemSet items = state->items;
				Array<Bool> *used = new (this) Array<Bool>(items.count(), false);
				for (Nat i = 0; i < items.count(); i++) {
					if (used->at(i))
						continue;
					used->at(i) = true;

					ProductionIter iter = items[i].iter(syntax);
					Token *token = iter.token();
					if (!token) {
						assert(iter.end());
						// Insert a reduce action.
						state->reduce->push(syntax->lookup->get(iter.production()));
					} else if (RuleToken *rule = as<RuleToken>(token)) {
						// Add new states to the goto-table.
						state->rules->put(rule->rule, createGoto(i, items, used, rule->rule));
					} else if (RegexToken *regex = as<RegexToken>(token)) {
						// Add a shift action.
						state->actions->push(createShift(i, items, used, regex));
					}
				}
			}

			Nat Table::createGoto(Nat start, ItemSet items, Array<Bool> *used, Rule *rule) {
				ItemSet result;
				{
					ProductionIter i = items[start].iter(syntax);
					result.push(syntax, i.nextA());
					result.push(syntax, i.nextB());
				}

				for (Nat i = start + 1; i < items.count(); i++) {
					if (used->at(i))
						continue;

					ProductionIter iter = items[i].iter(syntax);
					RuleToken *r = as<RuleToken>(iter.token());
					if (!r)
						continue;

					if (r->rule != rule)
						continue;

					used->at(i) = true;
					result.push(syntax, iter.nextA());
					result.push(syntax, iter.nextB());
				}

				return state(result.expand(syntax));
			}

			ShiftAction *Table::createShift(Nat start, ItemSet items, Array<Bool> *used, RegexToken *regex) {
				ItemSet result;
				{
					ProductionIter i = items[start].iter(syntax);
					result.push(syntax, i.nextA());
					result.push(syntax, i.nextB());
				}

				for (Nat i = start + 1; i < items.count(); i++) {
					if (used->at(i))
						continue;

					ProductionIter iter = items[i].iter(syntax);
					RegexToken *r = as<RegexToken>(iter.token());
					if (!r)
						continue;

					if (regex->regex != r->regex)
						continue;

					used->at(i) = true;
					result.push(syntax, iter.nextA());
					result.push(syntax, iter.nextB());
				}

				return new (this) ShiftAction(regex->regex, state(result.expand(syntax)));
			}

			void Table::toS(StrBuf *to) const {
				*to << L"Parse table, " << states->count() << L" states.\n";
				for (Nat i = 0; i < states->count(); i++) {
					*to << L"State " << i << L":\n";
					Indent z(to);

					states->at(i)->toS(to, syntax);
					*to << L"\n";
				}
			}

		}
	}
}
