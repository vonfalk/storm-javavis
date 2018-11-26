#include "stdafx.h"
#include "Parser.h"
#include "Compiler/Lib/Array.h"
#include "Utils/Memory.h"

// Debug the GLR parser? Causes performance penalties since we use a ::Indent object.
//#define GLR_DEBUG

// Backtrack to the last line-ending before doing error recovery?
#define GLR_BACKTRACK

namespace storm {
	namespace syntax {
		namespace glr {

			Parser::Parser() {
				syntax = new (this) Syntax();
				table = new (this) Table(syntax);
				emptyStr = new (this) Str(L"");
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
				initParse(root, str, file, start);
				doParse(startPos);
				finishParse();

				if (!acceptingStack)
					return false;

				return acceptingStack->required.empty();
			}

			/**
			 * Interesting cases to watch out for, and their solutions:
			 *
			 * Terminals matching long strings could be a problem. For example, if a string
			 * production is implemented as StrLiteral : "\"" - "[^\"\\]*" - "\"";, the middle
			 * regexp matches most of the rest of the file if the ending quote is missing. This is
			 * easily fixed in the grammar by not allowing the regex to match \n. If multiline
			 * string literals are desired, this can be handled on the parser level for better error
			 * recovery. The same holds true for multiline comment blocks. This could be solved in
			 * the parser by explicitly trying to do error recovery whenever a regex matches enough
			 * around a newline, but this is probably not a good idea as it will either incur a
			 * fairly large performance penalty on correct input or fail to kick in with near misses.
			 *
			 * Secondly, when performing shifts during error recovery, it is possible for the error
			 * recovery to simply decide to put the difficult parts of the input inside a comment
			 * (and because of the reason above, consuming too much of the input) and proceed
			 * happily. This is probably best solved by disallowing the parser to perform shifts
			 * which correspond to skipping the first terminal symbol in a production. Instead,
			 * these cases should be handled by completing entire productions prematurely and
			 * possibly skipping the offending characters using the error recovery rather than in a
			 * regex.
			 */
			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start) {
				initParse(root, str, file, start);

				// Start as usual.
				Set<StackItem *> *lastSet = null;
				Nat lastPos = 0;
				doParse(startPos, lastSet, lastPos);

				Nat length = source->peekLength();
				while (lastPos <= length && (acceptingStack == null || acceptingStack->pos < length)) {
					// Advance productions to accommodate for missing characters.
					stacks->set(0, lastSet);
					currentPos = lastPos;

					// Perform all possible shifts and reduces (regardless if they are possible or not).
					advanceAll();

					// Try to find a place in the input string where we can perform at least one
					// shift. We do not want to call 'doParse' here as that will try to re-do the
					// reduce operations we have already done in 'advanceAll()', which takes
					// time. 'actorShift()' only looks at the possible shifts, which is enough as we
					// have already done all possible reduces already.
					while (!actorShift() && currentPos < length) {
						// Advance to the next step in the input while keeping the current stack top.
						currentPos++;
						lastPos++;
					}

					// Advance from the current state and keep on parsing, as we now know that we
					// have something to do in the future.
					if (currentPos < length) {
						currentPos++;
						lastPos++;
						stacks->pop();
						doParse(currentPos, lastSet, lastPos);
					}
				}

				finishParse();
				if (hasTree()) {
					TreeNode node = store->at(acceptingStack->tree);
					// TODO: Look at any unfullfilled parent requirements as well!
					return node.errors();
				} else {
					return infoFailure();
				}
			}

			void Parser::initParse(Rule *root, Str *str, Url *file, Str::Iter start) {
				source = str;
				startPos = start.offset();
				sourceUrl = file;
				parseRoot = syntax->lookup(root);

				store = new (this) TreeStore(syntax);
				stacks = new (this) FutureStacks();
				acceptingStack = null;
				lastSet = null;
				lastPos = 0;
				visited = new (this) BoolSet();

				stacks->put(0, store, startState(startPos, root));
			}

			void Parser::doParse(Nat from) {
				Nat length = source->peekLength();
				for (Nat i = from; i <= length; i++) {
					Set<StackItem *> *top = stacks->top();

					// Process all states in 'top'.
					if (top)
						actor(i, top);

					// Advance one step.
					stacks->pop();
				}
			}

#ifdef GLR_BACKTRACK
			void Parser::doParse(Nat from, Set<StackItem *> *&states, Nat &pos) {
				states = null;
				Nat badness = 0;

				Nat newLine = from;
				Nat length = source->peekLength();
				for (Nat i = from; i <= length; i++) {
					Set<StackItem *> *top = stacks->top();

					// Process all states in 'top'.
					if (top && top->any()) {
						if (lastPos <= newLine) {
							// See if this line ending is better than the previous one. Give stack
							// tops close to the beginning of a line a disadvantage.
							Nat bad = newLine - lastPos;
							if (bad < badness || !states) {
								states = lastSet;
								pos = lastPos;
							} else {
								// Incur a penalty for being far away!
								badness += 2;
							}
						}
						newLine = false;
						actor(i, top);
					}

					// Advance one step.
					stacks->pop();

					if (source->c_str()[i] == '\n') {
						newLine = i;
					}
				}

				if (!states) {
					states = lastSet;
					pos = lastPos;
				}
			}
#else
			// Non-backtracking version.
			void Parser::doParse(Nat from, Set<StackItem *> *&states, Nat &pos) {
				doParse(from);

				states = lastSet;
				pos = lastPos;
			}
#endif

			void Parser::finishParse() {
#ifdef GLR_DEBUG
				PVAR(table);
#endif

				visited = null;
				stacks = null;

				// If we have multiple accepting stacks, find the one without requirements and put
				// it first!
				for (StackItem **prev = &acceptingStack; *prev; prev = &(*prev)->morePrev) {
					if ((*prev)->required.empty()) {
						// Put it first.
						StackItem *chosen = *prev;
						*prev = chosen->morePrev;
						acceptingStack = chosen;
					}
				}
			}

			void Parser::advanceAll() {
				// First: perform all reductions.
				reduceAll();
				// Then: perform all shifts. This ensures that we do not reduce the same production
				// at the same location in the syntax tree multiple times.
				shiftAll();
			}

			void Parser::reduceAll() {
				// NOTE: we need to use 'visited' as that one is also used in 'limitedReduce' for optimizations.
				visited->clear();
				Set<StackItem *> *src = stacks->top();

				bool any;
				do {
					any = false;
					for (Set<StackItem *>::Iter i = src->begin(), e = src->end(); i != e; ++i) {
						StackItem *now = i.v();
						if (visited->get(now->state))
							continue;
						visited->set(now->state, true);
						any = true;

						// Shift and reduce this state.
						ActorEnv env = {
							table->state(now->state),
							now,
							true,
						};
						actorReduceAll(env, null);
					}
				} while (any);
			}

			void Parser::shiftAll() {
				visited->clear();
				Set<StackItem *> *src = stacks->top();

				bool match = false;
				bool any = false;
				do {
					any = false;
					for (Set<StackItem *>::Iter i = src->begin(), e = src->end(); i != e; ++i) {
						StackItem *now = i.v();
						if (visited->get(now->state))
							continue;
						visited->set(now->state, true);
						any = true;

						match |= shiftAll(now);
					}
					// Stop as soon as we found a regex matching!
				} while (any && !match);
			}

			bool Parser::shiftAll(StackItem *now) {
				bool found = false;
				State *s = table->state(now->state);
				if (s->actions)
					found |= shiftAll(now, s->actions);
				if (s->rules)
					shiftAll(now, s->rules);

				return found;
			}

			bool Parser::shiftAll(StackItem *now, Array<Action> *actions) {
				bool found = false;

				// Check regular shifts.
				for (Nat i = 0; i < actions->count(); i++) {
					const Action &action = actions->at(i);

					// Stop at states that can actually be advanced in the current scenario. Regexes
					// matching epsilon are also let through.
					Nat match = action.regex.matchRaw(source, currentPos);
					if (match != Regex::NO_MATCH && match != currentPos) {
						found = true;
						continue;
					}

					InfoErrors errors = infoSuccess();
					// Note: this if-statement could be skipped (the body could always be
					// executed). This results in empty regexes being matched as parse errors even
					// when they are possibly not.
					if (!action.regex.matchesEmpty())
						errors += infoShifts(1);
					Nat tree = store->push(now->pos, errors).id();
					StackItem *item = new (this) StackItem(action.action, currentPos, now, tree);
					StackItem *topItem = stacks->top()->at(item);
					if (topItem == item) {
						// Inserted!
					} else {
						topItem->insert(store, item);
					}
				}

				return found;
			}

			void Parser::shiftAll(StackItem *now, Map<Nat, Nat> *actions) {
				// See if we can 'shift' through a nonterminal at this position.
				typedef Map<Nat, Nat>::Iter Iter;
				for (Iter i = actions->begin(), e = actions->end(); i != e; ++i) {
					Nat prod = i.k();
					Nat to = i.v();

					// Skip null terminals, they are already skipped.
					if (Syntax::specialProd(prod) == Syntax::prodESkip)
						continue;

					InfoErrors errors = infoSuccess();
					errors += infoShifts(Item(syntax, prod).length(syntax));
					Nat tree = store->push(now->pos, prod, errors, 0).id();
					StackItem *item = new (this) StackItem(to, currentPos, now, tree);
					StackItem *topItem = stacks->top()->at(item);
					if (topItem == item) {
						// Inserted!
					} else {
						topItem->insert(store, item);
					}
				}
			}

			bool Parser::actorShift() {
				bool any = false;
				Set<StackItem *> *src = stacks->top();

				for (Set<StackItem *>::Iter i = src->begin(), e = src->end(); i != e; ++i) {
					StackItem *now = i.v();

					ActorEnv env = {
						table->state(now->state),
						now,
						false,
					};


					// Array<Action> *actions = env.state->actions;
					// if (actions) {
					// 	for (Nat i = 0; i < actions->count(); i++) {
					// 		PVAR(actions->at(i).regex);
					// 	}
					// }

					any |= actorShift(env);
				}

				return any;
			}

			void Parser::actor(Nat pos, Set<StackItem *> *states) {
#ifdef GLR_DEBUG
				PVAR(pos);
#endif

				if (states->any()) {
					lastSet = states;
					lastPos = pos;
				}

				visited->clear();
				currentPos = pos;

				typedef Set<StackItem *>::Iter Iter;
				Bool done;
				do {
					done = true;
					for (Iter i = states->begin(), e = states->end(); i != e; ++i) {
						StackItem *now = i.v();
						if (visited->get(now->state))
							continue;
						visited->set(now->state, true);
						done = false;

						State *s = table->state(now->state);

						ActorEnv env = {
							s,
							now,
							false,
						};

						actorReduce(env, null);
						actorShift(env);
					}
				} while (!done);
			}

			bool Parser::actorShift(const ActorEnv &env) {
				Array<Action> *actions = env.state->actions;
				if (!actions)
					return false;

				bool any = false;
				for (Nat i = 0; i < actions->count(); i++) {
					const Action &action = actions->at(i);
					Nat matched = action.regex.matchRaw(source, currentPos);
					if (matched == Regex::NO_MATCH)
						continue;

					// Add the resulting match to the 'to-do' list if it matched more than zero characters.
					if (matched <= currentPos)
						continue;

					Nat offset = matched - currentPos;
					InfoErrors errors = infoSuccess();

					// Note: here, 'env.stack->pos' is used instead of 'currentPos'. Usually, they
					// are the same, but when error recovery kicks in 'env.stack->pos' may be
					// smaller, which means that some characters in the input were skipped. These
					// should be included somewhere so that the InfoTrees will have the correct length.
					Nat tree = store->push(env.stack->pos, errors).id();
					StackItem *item = new (this) StackItem(action.action, matched, env.stack, tree);
					stacks->put(offset, store, item);
					any = true;
#ifdef GLR_DEBUG
					PLN(L"Added " << item->state << L" with prev " << env.stack->state);
#endif
				}

				return any;
			}

			void Parser::actorReduce(const ActorEnv &env, StackItem *through) {
				if (env.reduceAll) {
					actorReduceAll(env, through);
					return;
				}

				Array<Nat> *toReduce = env.state->reduce;
				Nat count = toReduce->count();
				for (Nat i = 0; i < count; i++)
					doReduce(env, toReduce->at(i), through);

				// TODO: Put these inside 'toReduce' as well?
				Array<Action> *reduceEmpty = env.state->reduceOnEmpty;
				if (reduceEmpty) {
					for (Nat i = 0; i < reduceEmpty->count(); i++) {
						const Action &a = reduceEmpty->at(i);
						if (a.regex.matchRaw(source, currentPos) != currentPos)
							continue;

						doReduce(env, a.action, through);
					}
				}
			}

			void Parser::actorReduceAll(const ActorEnv &env, StackItem *through) {
				// See if we can reduce this one normally.
				Array<Nat> *toReduce = env.state->reduce;
				Nat count = toReduce->count();
				for (Nat i = 0; i < count; i++)
					doReduce(env, toReduce->at(i), through);

				// Do not reduce 'reduceOnEmpty' as we will skip over them using the 'shiftAll' function instead.
				// // Always reduce these if we're able to.
				// Array<Action> *reduceEmpty = env.state->reduceOnEmpty;
				// if (reduceEmpty) {
				// 	for (Nat i = 0; i < reduceEmpty->count(); i++) {
				// 		const Action &a = reduceEmpty->at(i);
				// 		doReduce(env, a.action, through);
				// 	}
				// }

				// See if we can reduce other items in this item set.
				ItemSet items = env.state->items;
				for (Nat i = 0; i < items.count(); i++) {
					Item item = items[i];

					// Already reduced in the ordinary manner?
					if (item.end())
						continue;

					// Do reduce!
					Nat rule = item.rule(syntax);
					Nat length = item.tokenPos(syntax);

#ifdef GLR_DEBUG
					PLN(L"Reducing " << item.toS(syntax) << L" early:");
#endif

					ReduceEnv re = {
						env,
						item.id,
						rule,
						length,
						item.length(syntax) - length,
					};
					reduce(re, env.stack, null, through, length);
				}
			}

			void Parser::doReduce(const ActorEnv &env, Nat production, StackItem *through) {
				Item item(syntax, production);
				Nat rule = item.rule(syntax);
				Nat length = item.length(syntax);

#ifdef GLR_DEBUG
				PLN(L"Reducing " << item.toS(syntax) << L":");
#endif

				// Do reductions.
				ReduceEnv re = {
					env,
					production,
					rule,
					length,
					0,
				};
				reduce(re, env.stack, null, through, length);
			}

			void Parser::reduce(const ReduceEnv &env, StackItem *stack, const Path *path, StackItem *through, Nat len) {
#ifdef GLR_DEBUG
				PLN(L"Reduce " << (void *)stack << L" " << stack->state << L" " << (void *)through << L" len " << len);
				::Indent z(util::debugStream());
#endif

				if (len > 0) {
					len--;

					Path next = {
						path,
						null,
					};

					// Keep on traversing...
					for (StackItem *i = stack; i; i = i->morePrev) {
						if (i->prev) {
							next.item = i;
							reduce(env, i->prev, &next, i == through ? null : through, len);
						}
					}
				} else if (through == null) {
					finishReduce(env, stack, path);
				}
			}

			void Parser::finishReduce(const ReduceEnv &env, StackItem *stack, const Path *path) {
				Engine &e = engine();
				State *state = table->state(stack->state);
				Map<Nat, Nat>::Iter to = state->rules->find(env.rule);

				bool accept = stack->prev == null && env.rule == parseRoot;
				bool reduce = to != state->rules->end();

				if (!accept && !reduce)
					return;

				// Create the syntax tree node for this reduction and compute required parent productions.
				ParentReq required;

				Bool usedNode = false;
				Bool nontermErrors = false;
				Nat node = 0;
				InfoErrors errors = infoSuccess();
				for (const Path *p = path; p; p = p->prev) {
					TreeNode now = store->at(p->item->tree);
					InfoErrors err = now.errors();
					errors += err;
					nontermErrors = now.leaf() & err.any();
					// TODO: Calling 'concat' in this manner is somewhat inefficient. It is better
					// to pre-compute the length once and for all and then do the concatenation
					// afterwards.
					required = required.concat(e, p->item->required);
				}
				errors += infoShifts(env.errors);
				if (env.errors || nontermErrors) {
					// Penalize characters in here appropriatly. Make sure to not count 'skipped'
					// multiple times while recursing.
					errors.chars(currentPos - stack->pos);
				}

				if (Syntax::specialProd(env.production) == Syntax::prodESkip) {
					// These are really just shifts.
					node = store->push(stack->pos, errors).id();
				} else {
					TreeNode fill = store->push(stack->pos, env.production, errors, env.length);
					TreeArray children = fill.children();
					const Path *top = path;
					for (Nat i = 0; i < env.length; i++) {
						children.set(i, top->item->tree);
						top = top->prev;
					}
					if (env.length > 0)
						fill.pos(store->at(children[0]).pos());

					node = fill.id();
				}

#ifdef GLR_DEBUG
				PVAR(accept);
				PVAR(reduce);
				PVAR(node);
#endif

				// Remove the currently reduced requirement.
				required = required.remove(e, syntax->parentId(env.rule));

				// Do we require any parent production being present? If a production requires
				// itself, that means it has to be nested. Ie. it does not immediately satisfy its
				// own requirement.
				required = required.concat(e, syntax->productionReq(env.production));


				// Accept?
				if (accept) {
					StackItem *add = new (e) StackItem(-1, currentPos, stack, node, required);

					if (acceptingStack && acceptingStack->pos == currentPos) {
						usedNode |= acceptingStack->insert(store, add, usedNode);
					} else {
						acceptingStack = add;
						usedNode = true;
					}
				}

				// Figure out which state to go to.
				if (reduce) {
					StackItem *add = new (e) StackItem(to.v(), currentPos, stack, node, required);
#ifdef GLR_DEBUG
					PLN(L"Added " << to.v() << L" with prev " << stack->state << L"(" << (void *)stack << L")");
#endif

					// Add the newly created state.
					Set<StackItem *> *top = stacks->top();
					StackItem *old = top->at(add);
					if (old == add) {
						// 'add' was successfully inserted. Nothing more to do!
						usedNode = true;
					} else {
						// We need to merge it with the old one.
						if (old->insert(store, add, usedNode)) {
							usedNode = true;
#ifdef GLR_DEBUG
							PLN(L"Inserted into " << old->state << L"(" << (void *)old << L")");
#endif
							// Note: 'add' is the actual link.
							limitedReduce(env, top, add);
						}
					}
				}

				if (!usedNode) {
					store->free(node);
				}
			}

			void Parser::limitedReduce(const ReduceEnv &env, Set<StackItem *> *top, StackItem *through) {
#ifdef GLR_DEBUG
				PLN(L"--LIMITED--");
#endif
				for (Set<StackItem *>::Iter i = top->begin(), e = top->end(); i != e; ++i) {
					StackItem *item = i.v();

					// Will this state be visited soon anyway?
					if (visited->get(item->state) == false)
						continue;

					State *state = table->state(item->state);

					ActorEnv aEnv = env.old;
					aEnv.state = state;
					aEnv.stack = item;
					actorReduce(aEnv, through);
				}
			}

			ItemSet Parser::startSet(Rule *root) {
				ItemSet r;

				RuleInfo *info = syntax->ruleInfo(syntax->lookup(root));
				for (Nat i = 0; i < info->count(); i++) {
					r.push(engine(), Item(syntax, info->at(i)));
				}

				return r.expand(syntax);
			}

			StackItem *Parser::startState(Nat pos, Rule *root) {
				return new (this) StackItem(table->state(startSet(root)), pos);
			}

			void Parser::clear() {
				store = null;
				stacks = null;
				source = null;
				sourceUrl = null;
				parseRoot = 0;
			}

			Bool Parser::hasTree() const {
				return acceptingStack != null && acceptingStack->tree != 0;
			}

			Str::Iter Parser::matchEnd() const {
				if (source && acceptingStack)
					return source->posIter(acceptingStack->pos);
				else
					return Str::Iter();
			}

			Bool Parser::hasError() const {
				if (!acceptingStack)
					return true;
				if (acceptingStack->required.any())
					return true;
				return acceptingStack->pos < source->peekLength();
			}

			Str *Parser::errorMsg() const {
				StrBuf *out = new (this) StrBuf();
				if (lastPos == source->peekLength())
					*out << L"Unexpected end of file.";
				else if (lastSet)
					errorMsg(out, lastPos, lastSet);
				else
					*out << L"No syntax provided.";

				return out->toS();
			}

			void Parser::errorMsg(StrBuf *out, Nat pos, Set<StackItem *> *states) const {
				Set<Str *> *errors = new (this) Set<Str *>();

				for (Set<StackItem *>::Iter i = states->begin(); i != states->end(); ++i) {
					errorMsg(errors, i.v()->state);
				}

				Str *ch = new (this) Str(source->posIter(pos).v());
				*out << L"Unexpected '" << ch->escape() << L"'.";

				if (errors->any()) {
					*out << L" Expected:";
					for (Set<Str *>::Iter i = errors->begin(); i != errors->end(); ++i) {
						*out << L"\n  " << i.v();
					}
				}
			}

			void Parser::errorMsg(Set<Str *> *errors, Nat state) const {
				ItemSet items = table->state(state)->items;

				for (Nat i = 0; i < items.count(); i++) {
					Item item = items.at(i);
					if (item.end())
						continue;

					if (item.isRule(syntax)) {
						errors->put(syntax->ruleName(item.nextRule(syntax)));
					} else {
						errors->put(TO_S(this, L"\"" << item.nextRegex(syntax) << L"\""));
					}
				}
			}

			SrcPos Parser::errorPos() const {
				return SrcPos(sourceUrl, lastPos);
			}

			Node *Parser::tree() const {
				if (acceptingStack == null)
					return null;
				if (acceptingStack->tree == 0)
					return null;

				return tree(store->at(acceptingStack->tree), acceptingStack->pos);
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

			void Parser::setToken(Node *result, TreeNode &node, Nat endPos, Token *token) const {
				if (token->target) {
					if (as<RegexToken>(token)) {
						Str::Iter from = source->posIter(node.pos());
						Str::Iter to = source->posIter(endPos);
						SrcPos pos(sourceUrl, node.pos());
						setValue(result, token->target, new (this) SStr(source->substr(from, to), pos));
					} else if (as<RuleToken>(token)) {
						setValue(result, token->target, tree(node, endPos));
					} else {
						assert(false, L"Unknown token type used for match.");
					}
				}
			}

			Node *Parser::tree(const TreeNode &node, Nat endPos) const {
				assert(node.children(), L"Trying to create a tree from a non-reduced node!");

				Production *p = syntax->production(node.production());
				Node *result = allocNode(p, node.pos());

				Item item = last(node.production());
				Nat pos = endPos;

				Nat repStart = node.pos();
				Nat repEnd = endPos;

				TreeArray children = node.children();
				for (Nat i = children.count(); i > 0; i--) {
					Nat childId = children[i-1];
					if (!item.prev(syntax))
						// Something is fishy...
						break;

					TreeNode child = store->at(childId);

					// Remember the capture!
					if (item.pos == p->repStart)
						repStart = child.pos();
					if (item.pos == p->repEnd)
						repEnd = child.pos();

					// Store any tokens in here.
					if (item.pos == Item::specialPos) {
						subtree(result, child, pos);
					} else {
						setToken(result, child, pos, p->tokens->at(item.pos));
					}

					pos = child.pos();
				}

				if (p->repCapture) {
					Str::Iter from = source->posIter(repStart);
					Str::Iter to = source->posIter(repEnd);
					SrcPos pos(sourceUrl, repStart);
					setValue(result, p->repCapture->target, new (this) SStr(source->substr(from, to), pos));
				}

				reverseNode(result);
				return result;
			}

			void Parser::subtree(Node *result, TreeNode &startNode, Nat endPos) const {
				assert(startNode.children(), L"Trying to create a subtree from a non-reduced node!");

				TreeNode *node = &startNode;
				Production *p = syntax->production(node->production());

				Item item = last(node->production());
				Nat pos = endPos;

				TreeArray children = startNode.children();
				Nat i = children.count();
				while (i > 0) {
					Nat childId = children[i-1];
					if (!item.prev(syntax))
						// Something is fishy...
						break;

					TreeNode child = store->at(childId);

					// Store any tokens in here.
					if (item.pos == Item::specialPos) {
						// This is always the first token in special productions, thus we can elliminate some recursion!
						assert(child.children(), L"Special production not properly reduced!");
						Nat reduced = child.production();
						if (Syntax::specialProd(reduced) == Syntax::prodRepeat) {
							assert(reduced == item.id, L"Special production has wrong recursion.");
							item = last(reduced);
							node = &child;
							children = child.children();
							i = children.count();
						} else {
							assert(Syntax::specialProd(reduced) == Syntax::prodEpsilon, L"Expected an epsilon production!");
							i--;
						}
					} else {
						setToken(result, child, pos, p->tokens->at(item.pos));

						// This shall not be done when we're elliminating recursion!
						pos = child.pos();
						i--;
					}
				}
			}

			Node *Parser::allocNode(Production *from, Nat pos) const {
				ProductionType *type = from->type();

				// A bit ugly, but this is enough for the object to be considered a proper object
				// when it is populated.
				void *mem = runtime::allocObject(type->size().current(), type);
				Node *r = new (Place(mem)) Node(SrcPos(sourceUrl, pos));
				type->vtable()->insert(r);

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
				if (!hasTree())
					return null;

				return infoTree(store->at(acceptingStack->tree), acceptingStack->pos);
			}

			InfoNode *Parser::infoTree(const TreeNode &node, Nat endPos) const {
				assert(node.children(), L"Trying to create a tree from a non-reduced node!");

				Production *p = syntax->production(node.production());
				Item item = last(node.production());
				Nat pos = endPos;
				Nat length = totalLength(node);
				Nat nodePos = length;
				InfoInternal *result = new (this) InfoInternal(p, length);
				InfoErrors errors = infoSuccess();

				if (p->indentType != indentNone)
					result->indent = new (this) InfoIndent(nodePos, nodePos, p->indentType);

				TreeArray children = node.children();

				// This production may be incomplete...
				while (item.tokenPos(p) > children.count())
					item.prev(p);

				for (Nat i = children.count(); i > 0; i--) {
					Nat childId = children[i-1];
					if (!item.prev(p))
						break;

					TreeNode child = store->at(childId);

					if (item.pos == Item::specialPos) {
						errors += infoSubtree(result, nodePos, child, pos);
					} else {
						errors += infoToken(result, nodePos, child, pos, p->tokens->at(item.pos));

						if (result->indent) {
							if (item.pos == p->indentStart)
								result->indent->start = nodePos;
							else if (item.pos == p->indentEnd)
								result->indent->end = nodePos;
						}
					}

					pos = child.pos();
				}

				assert(nodePos == 0, L"Node length computation was inaccurate!");

				if (errors != node.errors()) {
					result->error(true);
					// PLN(TO_S(this, errors << L" - " << node.errors() << L" in " << result->format()));
				}

				return result;
			}

			InfoErrors Parser::infoSubtree(InfoInternal *result, Nat &resultPos, TreeNode &startNode, Nat endPos) const {
				TreeNode *node = &startNode;
				Production *p = syntax->production(node->production());
				Item item = last(node->production());
				Nat pos = endPos;
				InfoErrors errors = infoSuccess();

				TreeArray children = node->children();

				// This production may be incomplete...
				while (item.tokenPos(p) > children.count())
					item.prev(p);

				Nat i = children.count();
				while (i > 0) {
					Nat childId = children[i-1];
					if (!item.prev(syntax))
						break;

					TreeNode child = store->at(childId);
					if (item.pos == Item::specialPos) {
						Nat reduced = child.production();
						if (Syntax::specialProd(reduced) == Syntax::prodRepeat) {
							item = last(reduced);
							node = &child;
							children = child.children();
							i = children.count();

							// This production may be incomplete...
							while (item.tokenPos(p) > children.count())
								item.prev(p);
						} else {
							i--;
						}
					} else {
						errors += infoToken(result, resultPos, child, pos, p->tokens->at(item.pos));

						if (result->indent) {
							if (item.pos == p->indentStart)
								result->indent->start = resultPos;
							else if (item.pos == p->indentEnd)
								result->indent->end = resultPos;
						}

						pos = child.pos();
						i--;
					}
				}

				return errors;
			}

			InfoErrors Parser::infoToken(InfoInternal *result, Nat &resultPos, TreeNode &node, Nat endPos, Token *token) const {
				assert(resultPos > 0, L"Too few children allocated in InfoNode!");

				InfoErrors errors = infoSuccess();
				InfoNode *created = null;
				if (node.children()) {
					if (Syntax::specialProd(node.production()) == Syntax::prodESkip) {
						// This is always an empty string! Even though it is a production, it should
						// look like a plain string.
						created = new (this) InfoLeaf(as<RegexToken>(token), emptyStr);
						// Move errors up to the parent.
						errors = infoSuccess();
					} else {
						created = infoTree(node, endPos);
						errors = node.errors();
						if (as<DelimToken>(token))
							created->delimiter(true);
					}
				} else {
					Str *str = emptyStr;
					if (node.pos() < endPos) {
						Str::Iter from = source->posIter(node.pos());
						Str::Iter to = source->posIter(endPos);
						str = source->substr(from, to);
					}
					created = new (this) InfoLeaf(as<RegexToken>(token), str);
					// Move errors up to the parent.
					errors = infoSuccess();
				}

				created->color = token->color;
				result->set(--resultPos, created);
				return errors;
			}

			Nat Parser::totalLength(const TreeNode &node) const {
				Item item = last(node.production());
				Nat length = item.length(syntax);

				TreeArray children = node.children();

				// This production may be incomplete.
				while (length > children.count()) {
					item.prev(syntax);
					length--;
				}

				for (Nat i = children.count(); i > 0; i--) {
					Nat childId = children[i-1];
					if (!item.prev(syntax))
						break;

					TreeNode child = store->at(childId);
					if (item.pos == Item::specialPos) {
						// Ignore this non-terminal.
						length--;

						// Traverse until we reach an epsilon production!
						for (TreeNode at = child;
							 at.children() && Syntax::specialProd(at.production()) == Syntax::prodRepeat;
							 at = store->at(at.children()[0])) {

							Item i(syntax, at.production());
							length += at.children().count();
							if (i.pos == Item::specialPos)
								length -= 1;
						}
					}
				}

				return length;
			}

			void Parser::completePrefix() {
				// This is a variant of the 'actor' function above, except that we only attempt to
				// perform reductions.
				typedef Set<StackItem *>::Iter Iter;

				visited->clear();
				Set<StackItem *> *top = stacks->top();

#ifdef GLR_DEBUG
				PLN(L"--- Starting error recovery at " << currentPos << "---");
#endif

				Bool done;
				do {
					done = true;
					for (Iter i = top->begin(), e = top->end(); i != e; ++i) {
						StackItem *now = i.v();
						if (visited->get(now->state))
							continue;
						visited->set(now->state, true);
						done = false;

						State *s = table->state(now->state);

						ActorEnv env = {
							s,
							now,
							true,
						};

						actorReduce(env, null);
					}
				} while (!done);
			}

			Nat Parser::stateCount() const {
				if (store)
					return store->count();
				else
					return 0;
			}

			Nat Parser::byteCount() const {
				if (store)
					return store->byteCount();
				else
					return 0;
			}

			void Parser::clearSyntax() {
				if (!table->empty())
					table = new (this) Table(syntax);
			}

		}
	}
}
