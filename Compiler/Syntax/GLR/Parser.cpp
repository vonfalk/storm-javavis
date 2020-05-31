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
				// table->populate();
				doParse(startPos);
				finishParse(null);

				if (!acceptingStack.any())
					return false;

				return acceptingStack.required().empty();
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
			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx) {
				initParse(root, str, file, start);

				// Start as usual.
				Array<Nat> *lastSet = null;
				Nat lastPos = 0;
				doParse(startPos, lastSet, lastPos);

				Nat length = source->peekLength();
				while (lastPos <= length && (!acceptingStack.any() || acceptingStack.pos() < length)) {
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

				finishParse(ctx);
				if (hasTree()) {
					TreeNode node = treeStore->at(acceptingStack.tree());
					// Count any unfullfilled requirements as additional shifts.
					return node.errors() + infoShifts(acceptingStack.required().count());
				} else {
					return infoFailure();
				}
			}

			void Parser::initParse(Rule *root, Str *str, Url *file, Str::Iter start) {
				source = str;
				startPos = start.offset();
				sourceUrl = file;
				parseRoot = syntax->lookup(root);

				treeStore = new (this) TreeStore(syntax);
				stackStore = new (this) StackStore(syntax->parentCount());
				stacks = new (this) FutureStacks(stackStore);
				acceptingStack = StackItem();
				lastSet = null;
				lastPos = 0;

				stacks->put(0, treeStore, startState(startPos, root));
			}

			void Parser::doParse(Nat from) {
				Nat length = source->peekLength();
				for (Nat i = from; i <= length; i++) {
					Array<Nat> *top = stacks->top();

					// Process all states in 'top'.
					if (top)
						actor(i, top);

					// Advance one step.
					stacks->pop();
				}
			}

#ifdef GLR_BACKTRACK
			void Parser::doParse(Nat from, Array<Nat> *&states, Nat &pos) {
				states = null;
				Nat badness = 0;

				Nat newLine = from;
				Nat length = source->peekLength();
				for (Nat i = from; i <= length; i++) {
					Array<Nat> *top = stacks->top();

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
			void Parser::doParse(Nat from, Array<Nat> *&states, Nat &pos) {
				doParse(from);

				states = lastSet;
				pos = lastPos;
			}
#endif

			void Parser::finishParse(MAYBE(InfoInternal *) context) {
#ifdef GLR_DEBUG
				PVAR(table);
#endif

				stacks = null;

				TODO(L"Fix this!");

				// See which productions are acceptable to us...
				StackItem ctxItem = stackStore->createItem();
				ItemReq ctx = ctxItem.required();
				for (InfoInternal *at = context; at; at = at->parent()) {
					ctx.add(syntax->parentId(syntax->lookup(at->production()->rule())));
				}

				// Copy the accepting stacks to a new StackStore to keep the memory footprint low.
				StackStore *nStore = new (this) StackStore(syntax->parentCount());

				// If we have multiple accepting stacks, find the ones without requirements and put
				// them first!
				// TODO: If no such stack exists, we want to pick the one with the least dependency errors.
				// This only affects the quality of highlighting in error cases, so it is fairly minor.
				StackItem first;
				StackItem insert;
				for (StackItem i = acceptingStack; i.any(); i = i.morePrev()) {
					if (ctx.any())
						i.required().remove(ctx);

					// Keep this first?
					if (i.required().empty()) {
						if (insert.any()) {
							StackItem next = nStore->createItem(i);
							next.morePrev(StackItem());
							insert.morePrev(next);
							insert = next;
						} else {
							first = insert = nStore->createItem(i);
							insert.morePrev(StackItem());
						}
					}
				}

				// Keep all others.
				for (StackItem i = acceptingStack; i.any(); i = i.morePrev()) {
					if (i.required().any()) {
						if (insert.any()) {
							StackItem next = nStore->createItem(i);
							next.morePrev(StackItem());
							insert.morePrev(next);
							insert = next;
						} else {
							first = insert = nStore->createItem(i);
							insert.morePrev(StackItem());
						}
					}
				}

				stackStore = nStore;
				acceptingStack = first;
			}

			void Parser::advanceAll() {
				// First: perform all reductions.
				reduceAll();
				// Then: perform all shifts. This ensures that we do not reduce the same production
				// at the same location in the syntax tree multiple times.
				shiftAll();
			}

			void Parser::reduceAll() {
				Array<Nat> *src = stacks->top();

				for (Nat i = 0; i < src->count(); i++) {
					StackItem now = stackStore->readItem(src->at(i));
					Nat state = now.state();
					topVisiting = i;

					// Shift and reduce this state.
					ActorEnv env = {
						table->state(state),
						now,
						true,
					};
					actorReduceAll(env, StackItem());
				}
			}

			void Parser::shiftAll() {
				Array<Nat> *src = stacks->top();

				bool match = false;
				Nat lastEnd = 0;
				do {
					Nat i = lastEnd;
					lastEnd = src->count();
					for (; i < lastEnd; i++) {
						StackItem now = stackStore->readItem(src->at(i));
						topVisiting = i;

						match |= shiftAll(now);
					}
					// Stop as soon as we found a regex matching!
				} while (lastEnd < src->count() && !match);
			}

			bool Parser::shiftAll(const StackItem &now) {
				bool found = false;
				State *s = table->state(now.state());
				if (s->actions)
					found |= shiftAll(now, s->actions);
				if (s->rules)
					shiftAll(now, s->rules);

				return found;
			}

			bool Parser::shiftAll(const StackItem &now, Array<Action> *actions) {
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
					Nat tree = treeStore->push(now.pos(), errors).id();
					StackItem item = stackStore->createItem(action.action, currentPos, now, tree);
					stacks->putFree(0, treeStore, item);
				}

				return found;
			}

			void Parser::shiftAll(const StackItem &now, Map<Nat, Nat> *actions) {
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
					Nat tree = treeStore->push(now.pos(), prod, errors, 0).id();
					StackItem item = stackStore->createItem(to, currentPos, now, tree);
					stacks->putFree(0, treeStore, item);
				}
			}

			bool Parser::actorShift() {
				bool any = false;
				Array<Nat> *src = stacks->top();

				for (Nat i = 0, count = src->count(); i < count; i++) {
					StackItem now = stackStore->readItem(src->at(i));
					ActorEnv env = {
						table->state(now.state()),
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

			void Parser::actor(Nat pos, Array<Nat> *states) {
				// PLN(L"At " << pos << L", stacks: " << states);
#ifdef GLR_DEBUG
				PVAR(pos);
#endif

				if (states->any()) {
					lastSet = states;
					lastPos = pos;
				}

				currentPos = pos;

				// Note: The 'states' array may grow while executing states!
				for (Nat i = 0; i < states->count(); i++) {
					StackItem now = stackStore->readItem(states->at(i));
					Nat state = now.state();
					topVisiting = i;

					State *s = table->state(state);

					ActorEnv env = {
						s,
						now,
						false,
					};

					actorReduce(env, StackItem());
					if (actorShift(env)) {
						// We need to keep this state.
						now.keepThis();
					}
				}

				// Free any states we don't need to keep.
				for (Nat i = 0, count = states->count(); i < count; i++) {
					StackItem now = stackStore->readItem(states->at(i));
					while (now.id() && !now.keep()) {
						StackItem next = now.morePrev();
						stackStore->free(now);
						now = next;
					}
				}
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
					Nat tree = treeStore->push(env.stack.pos(), errors).id();
					StackItem item = stackStore->createItem(action.action, matched, env.stack, tree);
					stacks->putFree(offset, treeStore, item);
					any = true;
#ifdef GLR_DEBUG
					PLN(L"Added " << item.state() << L" with prev " << env.stack.state());
#endif
				}

				return any;
			}

			void Parser::actorReduce(const ActorEnv &env, const StackItem &through) {
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

			void Parser::actorReduceAll(const ActorEnv &env, const StackItem &through) {
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

			void Parser::doReduce(const ActorEnv &env, Nat production, const StackItem &through) {
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

			void Parser::reduce(const ReduceEnv &env, const StackItem &stack,
								const Path *path, const StackItem &through, Nat len) {
#ifdef GLR_DEBUG
				PLN(L"Reduce " << stack.id() << L" " << stack.state() << L" " << through.id() << L" len " << len);
				::Indent z(util::debugStream());
#endif

				if (len > 0) {
					len--;

					Path next = {
						path,
						StackItem(),
					};

					// Keep on traversing...
					for (StackItem i = stack; i.any(); i = i.morePrev()) {
						StackItem prev = i.prev();
						if (prev.any()) {
							next.item = i;
							reduce(env, prev, &next, i.id() == through.id() ? StackItem() : through, len);
						}
					}
				} else if (!through.any()) {
					finishReduce(env, stack, path);
				}
			}

			void Parser::finishReduce(const ReduceEnv &env, const StackItem &stack, const Path *path) {
				State *state = table->state(stack.state());
				Map<Nat, Nat>::Iter to = state->rules->find(env.rule);

				bool accept = !stack.prev().any() && env.rule == parseRoot;
				bool reduce = to != state->rules->end();

				if (!accept && !reduce)
					return;

				// Note: We will update 'node' during the loop below.
				// We need to allocate this node here in order to have a ItemReq to update.
				StackItem add = stackStore->createItem(-1, currentPos, stack, 0);

				// Make sure we keep the "prev" stack.
				stack.keepThis();

				Bool usedNode = false;
				Bool nontermErrors = false;
				InfoErrors errors = infoSuccess();
				for (const Path *p = path; p; p = p->prev) {
					TreeNode now = treeStore->at(p->item.tree());
					InfoErrors err = now.errors();
					errors += err;
					nontermErrors = now.leaf() & err.any();
					add.required().concat(p->item.required());
				}
				errors += infoShifts(env.errors);
				if (env.errors || nontermErrors) {
					// Penalize characters in here appropriatly. Make sure to not count 'skipped'
					// multiple times while recursing.
					errors.chars(currentPos - stack.pos());
				}

				if (Syntax::specialProd(env.production) == Syntax::prodESkip) {
					// These are really just shifts.
					add.tree(treeStore->push(stack.pos(), errors).id());
				} else {
					TreeNode fill = treeStore->push(stack.pos(), env.production, errors, env.length);
					TreeArray children = fill.children();
					const Path *top = path;
					for (Nat i = 0; i < env.length; i++) {
						children.set(i, top->item.tree());
						top = top->prev;
					}
					if (env.length > 0)
						fill.pos(treeStore->at(children[0]).pos());

					add.tree(fill.id());
				}

#ifdef GLR_DEBUG
				PVAR(accept);
				PVAR(reduce);
				PVAR(add.tree());
#endif

				// Remove the currently reduced requirement.
				add.required().remove(syntax->parentId(env.rule));

				// Do we require any parent production being present? If a production requires
				// itself, that means it has to be nested. Ie. it does not immediately satisfy its
				// own requirement.
				add.required().add(syntax->productionReq(env.production));


				// Accept?
				if (accept) {
					StackItem acceptAdd = add;
					if (reduce)
						acceptAdd = stackStore->createItem(-1, currentPos, stack, add.tree(), add.required());

					if (acceptingStack.any() && acceptingStack.pos() == currentPos) {
						usedNode |= acceptingStack.insert(treeStore, acceptAdd, usedNode);
					} else {
						acceptingStack = acceptAdd;
						usedNode = true;
					}
				}

				// Figure out which state to go to.
				if (reduce) {
					add.state(to.v());
#ifdef GLR_DEBUG
					PLN(L"Added " << to.v() << L" with prev " << stack.state() << L"(" << stack.id() << L")");
#endif

					// Add the newly created state.
					StackItem old = stacks->putRaw(0, add);
					if (old.id() == add.id()) {
						// 'add' was successfully inserted. Nothing more to do!
						usedNode = true;
					} else {
						// We need to merge it with the old one.
						if (old.insert(treeStore, add, usedNode)) {
							usedNode = true;
#ifdef GLR_DEBUG
							PLN(L"Inserted into " << old.state() << L"(" << old.id() << L")");
#endif
							// Note: 'add' is the actual link.
							limitedReduce(env, add);
						}
					}
				}

				if (!usedNode) {
					treeStore->free(add.tree());
				}
			}

			void Parser::limitedReduce(const ReduceEnv &env, const StackItem &through) {
#ifdef GLR_DEBUG
				PLN(L"--LIMITED--");
#endif
				Array<Nat> *top = stacks->top();
				// We don't need to consider states after 'topVisiting', since they will be visited
				// by the actor soon anyway.
				for (Nat i = 0; i <= topVisiting; i++) {
					StackItem item = stackStore->readItem(top->at(i));
					State *state = table->state(item.state());

					ActorEnv aEnv = {
						state,
						item,
						env.old.reduceAll,
					};
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

			StackItem Parser::startState(Nat pos, Rule *root) {
				return stackStore->createItem(table->state(startSet(root)), pos);
			}

			void Parser::clear() {
				treeStore = null;
				stackStore = null;
				stacks = null;
				lastSet = null;
				source = null;
				sourceUrl = null;
				parseRoot = 0;
			}

			Bool Parser::hasTree() const {
				return acceptingStack.any() && acceptingStack.tree() != 0;
			}

			Str::Iter Parser::matchEnd() const {
				if (source && acceptingStack.any())
					return source->posIter(acceptingStack.pos());
				else
					return Str::Iter();
			}

			Bool Parser::hasError() const {
				if (!acceptingStack.any())
					return true;
				if (acceptingStack.required().any())
					return true;
				return acceptingStack.pos() < source->peekLength();
			}

			Str *Parser::errorMsg() const {
				StrBuf *out = new (this) StrBuf();
				if (acceptingStack.any() && acceptingStack.required().any())
					reqErrorMsg(out, acceptingStack);
				else if (lastPos == source->peekLength())
					*out << S("Unexpected end of file.");
				else if (lastSet)
					errorMsg(out, lastPos, lastSet);
				else
					*out << S("No syntax provided.");

				return out->toS();
			}

			void Parser::errorMsg(StrBuf *out, Nat pos, Array<Nat> *states) const {
				Set<Str *> *errors = new (this) Set<Str *>();

				for (Nat i = 0; i < states->count(); i++) {
					// TODO: We could get by with only the state ID:s!
					errorMsg(errors, stackStore->readItem(states->at(i)).state());
				}

				Str *ch = new (this) Str(source->posIter(pos).v());
				*out << S("Unexpected '") << ch->escape() << S("'.");

				if (errors->any()) {
					*out << S(" Expected:");
					for (Set<Str *>::Iter i = errors->begin(); i != errors->end(); ++i) {
						*out << S("\n  ") << i.v();
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
						errors->put(TO_S(this, S("\"") << item.nextRegex(syntax) << S("\"")));
					}
				}
			}

			void Parser::reqErrorMsg(StrBuf *out, const StackItem &state) const {
				for (StackItem item = state; item.any(); item = item.morePrev()) {
					// Create a copy of the item, so that we may modify the required set.
					StackItem copy = stackStore->createItem(item);

					// For each of these, we want to find a production with an unfullfilled requirement.
					Nat found = findMissingReq(item.tree(), copy.required());
					stackStore->free(copy);
					if (!found)
						continue;

					TreeNode node = treeStore->at(found);
					Production *p = syntax->production(node.production());
					// Should hold, since we found the production...
					if (!p->parent)
						continue;

					*out << S("The production ") << p->type()->identifier() << S(" (for the rule ")
						 << p->rule()->identifier() << S(") needs to be used inside the rule ")
						 << p->parent->identifier() << S(".");
				}
			}

			Nat Parser::findMissingReq(Nat tree, ItemReq required) const {
				TreeNode node = treeStore->at(tree);

				// Leaf nodes are not interesting. They're just shifts.
				if (node.leaf())
					return 0;

				TreeArray children = node.children();

				Nat production = children.production();
				Nat rule = Item(syntax, production).rule(syntax);

				// Is this the node we're looking for?
				ReqId here = syntax->productionReq(production);
				if (here.any() && required.has(here)) {
					return tree;
				}

				// If this rule was used as a dependency, remove it from the set now since it will
				// fulfill that requirement of its children.
				required.remove(syntax->parentId(rule));

				// Search our children. Pick the first match.
				for (Nat i = 0; i < children.count(); i++) {
					if (Nat found = findMissingReq(children[i], required))
						return found;
				}

				return 0;
			}

			SrcPos Parser::errorPos() const {
				// See if we need to examine stacks and such. This condition and the contained loop
				// should match what's in 'reqErrorMsg'.
				if (acceptingStack.any() && acceptingStack.required().any()) {
					for (StackItem item = acceptingStack; item.any(); item = item.morePrev()) {
						Nat found = findMissingReq(item.tree(), item.required());
						if (found) {
							// Just pick the first one!
							Nat pos = treeStore->at(found).pos();
							return SrcPos(sourceUrl, pos, pos + 1);
						}
					}
				}

				// Otherwise, it's just 'lastPos'.
				return SrcPos(sourceUrl, lastPos, lastPos + 1);
			}

			Node *Parser::tree() const {
				if (!hasTree())
					return null;

				return tree(treeStore->at(acceptingStack.tree()), acceptingStack.pos());
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
					if (token->asRegex()) {
						Str::Iter from = source->posIter(node.pos());
						Str::Iter to = source->posIter(endPos);
						SrcPos pos(sourceUrl, node.pos(), endPos);
						setValue(result, token->target, new (this) SStr(source->substr(from, to), pos));
					} else if (token->asRule()) {
						setValue(result, token->target, tree(node, endPos));
					} else {
						assert(false, L"Unknown token type used for match.");
					}
				}
			}

			Node *Parser::tree(const TreeNode &node, Nat endPos) const {
				assert(node.children(), L"Trying to create a tree from a non-reduced node!");

				Production *p = syntax->production(node.production());
				Node *result = allocNode(p, node.pos(), endPos);

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

					TreeNode child = treeStore->at(childId);

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
					SrcPos pos(sourceUrl, repStart, repEnd);
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

					TreeNode child = treeStore->at(childId);

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

			Node *Parser::allocNode(Production *from, Nat start, Nat end) const {
				ProductionType *type = from->type();

				// A bit ugly, but this is enough for the object to be considered a proper object
				// when it is populated.
				void *mem = runtime::allocObject(type->size().current(), type);
				Node *r = new (Place(mem)) Node(SrcPos(sourceUrl, start, end));
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

				return infoTree(treeStore->at(acceptingStack.tree()), acceptingStack.pos());
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

					TreeNode child = treeStore->at(childId);

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

					TreeNode child = treeStore->at(childId);
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
						created = new (this) InfoLeaf(token->asRegex(), emptyStr);
						// Move errors up to the parent.
						errors = infoSuccess();
					} else {
						created = infoTree(node, endPos);
						errors = node.errors();
						if (token->asDelim())
							created->delimiter(true);
					}
				} else {
					Str *str = emptyStr;
					if (node.pos() < endPos) {
						Str::Iter from = source->posIter(node.pos());
						Str::Iter to = source->posIter(endPos);
						str = source->substr(from, to);
					}
					created = new (this) InfoLeaf(token->asRegex(), str);
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

					TreeNode child = treeStore->at(childId);
					if (item.pos == Item::specialPos) {
						// Ignore this non-terminal.
						length--;

						// Traverse until we reach an epsilon production!
						for (TreeNode at = child;
							 at.children() && Syntax::specialProd(at.production()) == Syntax::prodRepeat;
							 at = treeStore->at(at.children()[0])) {

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

				Array<Nat> *top = stacks->top();

#ifdef GLR_DEBUG
				PLN(L"--- Starting error recovery at " << currentPos << "---");
#endif

				for (Nat i = 0; i < top->count(); i++) {
					StackItem now = stackStore->readItem(top->at(i));
					Nat state = now.state();
					topVisiting = i;

					State *s = table->state(state);

					ActorEnv env = {
						s,
						now,
						true,
					};

					actorReduce(env, StackItem());
				}
			}

			Nat Parser::stateCount() const {
				if (treeStore)
					return treeStore->count();
				else
					return 0;
			}

			Nat Parser::byteCount() const {
				Nat count = 0;

				if (treeStore)
					count += treeStore->byteCount();

				if (stackStore)
					count += stackStore->byteCount();

				return count;
			}

			void Parser::clearSyntax() {
				if (!table->empty())
					table = new (this) Table(syntax);
			}

		}
	}
}
