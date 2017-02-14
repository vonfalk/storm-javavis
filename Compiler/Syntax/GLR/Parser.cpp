#include "stdafx.h"
#include "Parser.h"

namespace storm {
	namespace syntax {
		namespace glr {

			Parser::Parser() {
				syntax = new (this) Syntax();
				table = new (this) Table(syntax);
			}

			void Parser::add(Rule *rule) {
				syntax->add(rule);
			}

			void Parser::add(ProductionType *type) {
				clearSyntax();
				syntax->add(type->production);
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

			Bool Parser::sameSyntax(ParserBackend *other) {
				if (runtime::typeOf(this) != runtime::typeOf(other))
					return false;
				Parser *o = (Parser *)other;
				return syntax->sameSyntax(o->syntax);
			}

			Bool Parser::parse(Rule *root, Str *str, Url *file, Str::Iter start) {
				PVAR(str);
				source = str;
				sourceUrl = file;
				parseRoot = syntax->lookup(root);

				stacks = new (this) FutureStacks();
				stacks->put(0, startState(root));
				acceptingStack = null;

				BoolSet *processed = new (this) BoolSet();

				// Go through states until we reach the end of file.
				Nat length = str->peekLength();
				for (Nat i = start.offset(); i <= length; i++) {
					Set<StackItem *> *top = stacks->top();

					// Process all states in 'top' until none remain.
					if (top)
						actor(i, top, processed);

					// Advance one step.
					stacks->pop();
				}

				PVAR(acceptingStack);
				PVAR(table);
				return acceptingStack != null;
			}

			void Parser::actor(Nat pos, Set<StackItem *> *states, BoolSet *processed) {
				processed->clear();

				Bool done;
				do {
					done = true;

					for (Set<StackItem *>::Iter i = states->begin(), e = states->end(); i != e; ++i) {
						StackItem *now = i.v();
						if (processed->get(now->state))
							continue;
						processed->set(now->state, true);
						done = false;
						State *s = table->state(now->state);

						// Actor actions.
						actorShift(pos, s, now);
						actorReduce(pos, s, now, null);
					}
				} while (!done);
			}

			void Parser::actorShift(Nat pos, State *state, StackItem *stack) {
				// NOTE: We need to properly handle shift actions matching the empty string.

				Array<ShiftAction *> *actions = state->actions;
				if (!actions)
					return;

				for (Nat i = 0; i < actions->count(); i++) {
					ShiftAction *action = actions->at(i);
					Nat matched = action->regex.matchRaw(source, pos);
					if (matched == Regex::NO_MATCH)
						continue;

					// Add the resulting match to the 'to-do' list.
					Nat offset = matched - pos;
					stacks->put(offset, new (this) StackItem(action->state, stack));
				}
			}

			void Parser::actorReduce(Nat pos, State *state, StackItem *stack, StackItem *through) {
				Array<Nat> *reduce = state->reduce;
				if (!reduce)
					return;

				for (Nat i = 0; i < reduce->count(); i++) {
					Nat id = reduce->at(i);

					Item item(syntax, id);
					Nat rule = item.rule(syntax);

					// Do reductions.
					ReduceEnv env = {
						pos,
						stack,
						rule,
					};
					this->reduce(env, stack, through, item.length(syntax));
				}
			}

			void Parser::reduce(const ReduceEnv &env, StackItem *stack, StackItem *through, Nat len) {
				if (len > 0) {
					if (stack == through)
						through = null;

					// Keep on traversing...
					len--;
					for (StackItem *i = stack; i; i = i->morePrev) {
						if (i->prev)
							reduce(env, i->prev, through, len);
					}
				} else if (through == null) {
					if (stack->prev == null && env.rule == parseRoot) {
						// TODO: Choose the best match if multiple ones exist.
						acceptingStack = env.oldTop;
						acceptingPos = env.pos;
					}

					// Figure out which state to go to.
					State *state = table->state(stack->state);
					Map<Rule *, Nat>::Iter to = state->rules->find(env.rule);
					if (to != state->rules->end()) {
						StackItem *add = new (this) StackItem(to.v(), stack);
						add->reduced = env.oldTop;

						// Add the newly created state.
						Set<StackItem *> *top = stacks->top();
						StackItem *old = top->at(add);
						if (old == add) {
							// 'add' was successfully inserted. Nothing more to do!
						} else {
							// We need to merge it with the old one.
							if (old->insert(add)) {
								// Note: we should really make sure we visit both 'stack' and 'add'...
								limitedReduce(env, top, stack);
							}
						}
					}
				}
			}

			void Parser::limitedReduce(const ReduceEnv &env, Set<StackItem *> *top, StackItem *through) {
				for (Set<StackItem *>::Iter i = top->begin(), e = top->end(); i != e; ++i) {
					StackItem *item = i.v();
					State *state = table->state(item->state);

					actorReduce(env.pos, state, item, through);
				}
			}

			ItemSet Parser::startSet(Rule *root) {
				ItemSet r;

				RuleInfo info = syntax->ruleInfo(syntax->lookup(root));
				for (Nat i = 0; i < info.count(); i++) {
					r.push(engine(), Item(syntax, info[i]));
				}

				return r.expand(syntax);
			}

			StackItem *Parser::startState(Rule *root) {
				return new (this) StackItem(table->state(startSet(root)));
			}

			void Parser::clear() {
				stacks = null;
				source = null;
				sourceUrl = null;
				parseRoot = null;
			}

			Bool Parser::hasError() const {
				return false;
			}

			Bool Parser::hasTree() const {
				return acceptingStack != null;
			}

			Str::Iter Parser::matchEnd() const {
				if (source && hasTree())
					return source->posIter(acceptingPos);
				else
					return Str::Iter();
			}

			Str *Parser::errorMsg() const {
				return null;
			}

			SrcPos Parser::errorPos() const {
				return SrcPos();
			}

			Node *Parser::tree() const {
				return null;
			}

			InfoNode *Parser::infoTree() const {
				return null;
			}

			Nat Parser::stateCount() const {
				return 0;
			}

			Nat Parser::byteCount() const {
				return 0;
			}

			void Parser::clearSyntax() {
				if (!table->empty())
					table = new (this) Table(syntax);
			}

		}
	}
}
