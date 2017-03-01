#include "stdafx.h"
#include "Table.h"
#include "Exception.h"

namespace storm {
	namespace syntax {
		namespace glr {

			/**
			 * Action.
			 */

			Action::Action(Regex regex, Nat state) : regex(regex), state(state) {}

			StrBuf &operator <<(StrBuf &to, Action action) {
				return to << L"\"" << action.regex << L"\" -> " << action.state;
			}

			/**
			 * State.
			 */

			State::State(ItemSet items) : items(items) {}

			void State::toS(StrBuf *to) const {
				toS(to, null);
			}

			void State::toS(StrBuf *to, Syntax *syntax) const {
				if (actions && rules) {
					*to << L"Actions:\n";
					Indent z(to);
					for (Nat i = 0; i < actions->count(); i++)
						*to << L"shift " << actions->at(i) << L"\n";

					for (Map<Nat, Nat>::Iter i = rules->begin(), end = rules->end(); i != end; ++i)
						*to << L"goto " << syntax->ruleName(i.k()) << L" -> " << i.v() << L"\n";

					for (Set<Nat>::Iter i = reduce->begin(), e = reduce->end(); i != e; ++i)
						*to << L"reduce " << Item(syntax, i.v()).toS(syntax) << L"\n";

					for (Nat i = 0; i < reduceLookahead->count(); i++) {
						Action a = reduceLookahead->at(i);
						*to << L"reduce " << Item(syntax, a.state).toS(syntax) << L" on \"" << a.regex << L"\"\n";
					}

					for (Nat i = 0; i < reduceOnEmpty->count(); i++) {
						Action a = reduceOnEmpty->at(i);
						*to << L"reduce " << Item(syntax, a.state).toS(syntax) << L" when \"" << a.regex << L"\" is empty\n";
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
				lookup->put(s, id);
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
				state->actions = new (this) Array<Action>();
				state->rules = new (this) Map<Nat, Nat>();
				state->reduce = new (this) Set<Nat>();
				state->reduceLookahead = new (this) Array<Action>();
				state->reduceOnEmpty = new (this) Array<Action>();

				// TODO: We might want to optimize this in the future. Currently we are using O(n^2)
				// time, but this can be done in O(n) time using a hash map, but that is probably
				// overkill for < 40 elements or so.

				ItemSet items = state->items;
				Array<Bool> *used = new (this) Array<Bool>(items.count(), false);
				for (Nat i = 0; i < items.count(); i++) {
					if (used->at(i))
						continue;
					used->at(i) = true;

					Item item = items[i];
					if (item.end()) {
						// Insert a reduce action.
						state->reduce->put(item.id);

						Set<Regex> *follows = syntax->ruleInfo(item.rule(syntax))->follows(syntax);
						for (Set<Regex>::Iter i = follows->begin(), e = follows->end(); i != e; ++i) {
							state->reduceLookahead->push(Action(i.v(), item.id));
						}
					} else if (item.isRule(syntax)) {
						// Add new states to the goto-table.
						Nat rule = item.nextRule(syntax);
						state->rules->put(rule, createGoto(i, items, used, rule));
					} else {
						// Add a shift action.
						Action shift = createShift(i, items, used, item.nextRegex(syntax));
						state->actions->push(shift);

						if (shift.regex.matchesEmpty()) {
							// We need to pretend a production can be reduced here!
							Nat rule = Syntax::prodESkip | shift.state;
							state->rules->put(rule, shift.state);

							// We need to add the reduction as well.
							state->reduceOnEmpty->push(Action(shift.regex, rule));
						}
					}
				}
			}

			Nat Table::createGoto(Nat start, ItemSet items, Array<Bool> *used, Nat rule) {
				ItemSet result;
				result.push(engine(), items[start].next(syntax));

				for (Nat i = start + 1; i < items.count(); i++) {
					if (used->at(i))
						continue;

					Item item = items[i];
					if (item.end() || !item.isRule(syntax))
						continue;

					if (item.nextRule(syntax) != rule)
						continue;

					used->at(i) = true;
					result.push(engine(), item.next(syntax));
				}

				return state(result.expand(syntax));
			}

			Action Table::createShift(Nat start, ItemSet items, Array<Bool> *used, Regex regex) {
				ItemSet result;
				result.push(engine(), items[start].next(syntax));

				for (Nat i = start + 1; i < items.count(); i++) {
					if (used->at(i))
						continue;

					Item item = items[i];
					if (item.end() || item.isRule(syntax))
						continue;

					Regex r = item.nextRegex(syntax);
					if (r != regex)
						continue;

					used->at(i) = true;
					result.push(engine(), item.next(syntax));
				}

				return Action(regex, state(result.expand(syntax)));
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
