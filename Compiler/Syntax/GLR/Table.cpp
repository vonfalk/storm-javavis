#include "stdafx.h"
#include "Table.h"
#include "Exception.h"

namespace storm {
	namespace syntax {
		namespace glr {

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
