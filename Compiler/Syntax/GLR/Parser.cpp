#include "stdafx.h"
#include "Parser.h"
#include "Compiler/Lib/Array.h"
#include "Utils/Memory.h"

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
				source = str;
				sourceUrl = file;
				parseRoot = syntax->lookup(root);

				stacks = new (this) FutureStacks();
				acceptingStack = null;

				BoolSet *processed = new (this) BoolSet();
				Nat startPos = start.offset();
				Nat length = str->peekLength();

				// Go through states until we reach the end of file.
				stacks->put(0, startState(startPos, root));
				for (Nat i = startPos; i <= length; i++) {
					Set<StackItem *> *top = stacks->top();

					// Process all states in 'top' until none remain.
					if (top)
						actor(i, top, processed);

					// Advance one step.
					stacks->pop();
				}

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
					stacks->put(offset, new (this) StackItem(action->state, matched, stack));
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
						id,
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
						acceptingStack = new (this) StackItem(-1, env.pos, stack, env.oldTop, env.production);
					}

					// Figure out which state to go to.
					State *state = table->state(stack->state);
					Map<Nat, Nat>::Iter to = state->rules->find(env.rule);
					if (to != state->rules->end()) {
						StackItem *add = new (this) StackItem(to.v(), env.pos, stack, env.oldTop, env.production);

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

			StackItem *Parser::startState(Nat pos, Rule *root) {
				return new (this) StackItem(table->state(startSet(root)), pos);
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
				if (source && acceptingStack)
					return source->posIter(acceptingStack->pos);
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
				if (acceptingStack == null)
					return null;

				return tree(acceptingStack);
			}

			template <class T>
			static void setValue(Node *node, MemberVar *target, T *elem) {
				int offset = target->offset().current();
				if (isArray(target->type)) {
					// Arrays are initialized earlier.
					OFFSET_IN(node, offset, Array<T *> *)->push(elem);
				} else {
					OFFSET_IN(node, offset, T *) = elem;
				}
			}

			static void reverseNode(Node *node) {
				ProductionType *t = (ProductionType *)runtime::typeOf(node);

				for (Nat i = 0; i < t->arrayMembers->count(); i++) {
					MemberVar *v = t->arrayMembers->at(i);
					int offset = v->offset().current();
					Array<Object *> *array = OFFSET_IN(node, offset, Array<Object *> *);
					array->reverse();
				}
			}

			void Parser::setToken(Node *result, StackItem *at, StackItem *prev, Token *token) const {
				if (token->target) {
					Object *match = null;
					if (as<RegexToken>(token)) {
						Str::Iter from = source->posIter(prev->pos);
						Str::Iter to = source->posIter(at->pos);
						SrcPos pos(sourceUrl, prev->pos);
						setValue(result, token->target, new (this) SStr(source->substr(from, to), pos));
					} else if (as<RuleToken>(token)) {
						setValue(result, token->target, tree(at));
					} else {
						assert(false, L"Unknown token type used for match.");
					}
				}
			}

			Node *Parser::tree(StackItem *top) const {
				assert(top->reduced, L"Trying to create a tree from a non-reduced node!");

				Item item = last(top->reducedId);
				Production *p = syntax->production(top->reducedId);
				Node *result = allocNode(p, top->prev->pos);

				// Set to the start position as we miss 'repStart' when it is at offset 0.
				Nat repStart = top->prev->pos;
				Nat repEnd = 0;

				for (StackItem *at = top->reduced; at != top->prev; at = at->prev) {
					StackItem *prev = at->prev;
					if (!item.prev(p))
						continue;
					assert(item.pos != Item::endPos);

					// Remember the capture!
					if (item.pos == p->repStart)
						repStart = at->pos;
					if (item.pos == p->repEnd || item.pos == Item::specialPos)
						repEnd = at->pos;

					if (item.pos == Item::specialPos) {
						subtree(result, at);
					} else {
						setToken(result, at, prev, p->tokens->at(item.pos));
					}
				}

				// Store the capture!
				if (p->repCapture && p->repCapture->target && repStart <= repEnd) {
					Str::Iter from = source->posIter(repStart);
					Str::Iter to = source->posIter(repEnd);
					SrcPos pos(sourceUrl, repStart);
					setValue(result, p->repCapture->target, new (this) SStr(source->substr(from, to), pos));
				}

				reverseNode(result);
				return result;
			}

			void Parser::subtree(Node *result, StackItem *top) const {
				assert(top->reduced, L"Trying to create a subtree from a non-reduced node!");

				Item item = last(top->reducedId);
				Production *p = syntax->production(top->reducedId);

				for (StackItem *at = top->reduced; at != top->prev; at = at->prev) {
					StackItem *prev = at->prev;
					if (!item.prev(p))
						continue;

					if (item.pos == Item::specialPos) {
						// This is always the first one (if present). To reduce stack space
						// requirements, restart wit the new production directly!
						if (Syntax::specialProd(at->reducedId) == Syntax::prodRepeat) {
							assert(at->reducedId == top->reducedId);
							top = at;
							item = last(top->reducedId);
							at = top->reduced;
						} else {
							// Should be the epsilon production.
							assert(Syntax::specialProd(at->reducedId) == Syntax::prodEpsilon);
						}
					} else {
						setToken(result, at, prev, p->tokens->at(item.pos));
					}
				}
			}

			Node *Parser::allocNode(Production *from, Nat pos) const {
				ProductionType *type = from->type();

				// A bit ugly, but this is enough for the object to be considered a proper object
				// when it is populated.
				void *mem = runtime::allocObject(type->size().current(), type);
				Node *r = new (Place(mem)) Node(SrcPos(sourceUrl, pos));
				type->vtable->insert(r);

				// Create any arrays needed.
				for (nat i = 0; i < type->arrayMembers->count(); i++) {
					MemberVar *v = type->arrayMembers->at(i);
					int offset = v->offset().current();

					// This will actually create the correct subtype as long as we're creating
					// something inherited from Object or TObject (which we are).
					Array<Object *> *arr = new (v->type.type) Array<Object *>();
					OFFSET_IN(r, offset, Object *) = arr;
				}

				return r;
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
