#include "stdafx.h"
#include "Parser.h"
#include "Compiler/Lib/Array.h"
#include "Utils/Memory.h"

// Debug the GLR parser? Causes performance penalties since we use a ::Indent object.
//#define GLR_DEBUG

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
				source = str;
				sourceUrl = file;
				parseRoot = syntax->lookup(root);

				store = new (this) TreeStore(syntax);
				stacks = new (this) FutureStacks();
				acceptingStack = null;
				lastSet = null;
				lastPos = 0;
				visited = new (this) BoolSet();

				startPos = start.offset();
				Nat length = str->peekLength();

				// Go through states until we reach the end of file.
				stacks->put(0, store, startState(startPos, root));
				for (Nat i = startPos; i <= length; i++) {
					Set<StackItem *> *top = stacks->top();

					// Process all states in 'top' until none remain.
					if (top)
						actor(i, top);

					// Advance one step.
					stacks->pop();
				}

#ifdef GLR_DEBUG
				PVAR(table);
#endif

				visited = null;
				stacks = null;

				return acceptingStack != null;
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

			void Parser::actorShift(const ActorEnv &env) {
				Array<Action> *actions = env.state->actions;
				if (!actions)
					return;

				for (Nat i = 0; i < actions->count(); i++) {
					const Action &action = actions->at(i);
					Nat matched = action.regex.matchRaw(source, currentPos);
					if (matched == Regex::NO_MATCH)
						continue;

					// Add the resulting match to the 'to-do' list if it matched more than zero characters.
					if (matched <= currentPos)
						continue;

					Nat offset = matched - currentPos;
					Nat tree = store->push(currentPos).id();
					StackItem *item = new (this) StackItem(action.action, matched, env.stack, tree);
					stacks->put(offset, store, item);
#ifdef GLR_DEBUG
					PLN(L"Added " << item->state << L" with prev " << env.stack->state);
#endif
				}
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
				for (nat i = 0; i < count; i++)
					doReduce(env, toReduce->at(i), through);

				// Always reduce these if we're able to.
				Array<Action> *reduceEmpty = env.state->reduceOnEmpty;
				if (reduceEmpty) {
					for (Nat i = 0; i < reduceEmpty->count(); i++) {
						const Action &a = reduceEmpty->at(i);
						doReduce(env, a.action, through);
					}
				}

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
							next.treeNode = i->tree;
							reduce(env, i->prev, &next, i == through ? null : through, len);
						}
					}
				} else if (through == null) {
					finishReduce(env, stack, path);
				}
			}

			void Parser::finishReduce(const ReduceEnv &env, StackItem *stack, const Path *path) {
				State *state = table->state(stack->state);
				Map<Nat, Nat>::Iter to = state->rules->find(env.rule);

				bool accept = stack->prev == null && env.rule == parseRoot;
				bool reduce = to != state->rules->end();

				if (!accept && !reduce)
					return;

				// Create the syntax tree node for this reduction.

				Nat node = 0;
				if (Syntax::specialProd(env.production) == Syntax::prodESkip) {
					// These are really just shifts.
					node = store->push(currentPos).id();
				} else {
					TreeNode fill = store->push(currentPos, env.production, env.length);
					TreeArray children = fill.children();
					const Path *top = path;
					for (Nat i = 0; i < env.length; i++) {
						children.set(i, top->treeNode);
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

				if (accept) {
					StackItem *add = new (this) StackItem(-1, currentPos, stack, node);

					if (acceptingStack && acceptingStack->pos == currentPos) {
						acceptingStack->insert(store, add);
					} else {
						acceptingStack = add;
					}
				}

				// Figure out which state to go to.
				if (reduce) {
					StackItem *add = new (this) StackItem(to.v(), currentPos, stack, node);
#ifdef GLR_DEBUG
					PLN(L"Added " << to.v() << L" with prev " << stack->state << L"(" << (void *)stack << L")");
#endif

					// Add the newly created state.
					Set<StackItem *> *top = stacks->top();
					StackItem *old = top->at(add);
					if (old == add) {
						// 'add' was successfully inserted. Nothing more to do!
					} else {
						// We need to merge it with the old one.
						if (old->insert(store, add)) {
#ifdef GLR_DEBUG
							PLN(L"Inserted into " << old->state << L"(" << (void *)old << L")");
#endif
							// Note: 'add' is the actual link.
							limitedReduce(env, top, add);
						}
					}
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
				return acceptingStack != null;
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

				*out << L"Unexpected '" << source->posIter(pos).v() << L"'.";

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
				if (acceptingStack->tree == null)
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
					Object *match = null;
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

			Node *Parser::tree(TreeNode &node, Nat endPos) const {
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

					TreeNode &child = store->at(childId);

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

					TreeNode &child = store->at(childId);

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
				if (acceptingStack == null)
					return null;
				if (acceptingStack->tree == null)
					return null;

				return infoTree(store->at(acceptingStack->tree), acceptingStack->pos);
			}

			InfoNode *Parser::infoTree(TreeNode &node, Nat endPos) const {
				assert(node.children(), L"Trying to create a tree from a non-reduced node!");

				Production *p = syntax->production(node.production());
				Item item = last(node.production());
				Nat pos = endPos;
				Nat length = totalLength(node);
				Nat nodePos = length;
				InfoInternal *result = new (this) InfoInternal(p, length);

				if (p->indentType != indentNone)
					result->indent = new (this) InfoIndent(0, nodePos, p->indentType);

				TreeArray children = node.children();

				// This production may be incomplete...
				while (item.tokenPos(p) > children.count())
					item.prev(p);

				for (Nat i = children.count(); i > 0; i--) {
					Nat childId = children[i-1];
					if (!item.prev(p))
						break;

					TreeNode &child = store->at(childId);

					if (item.pos == Item::specialPos) {
						infoSubtree(result, nodePos, child, pos);
					} else {
						infoToken(result, nodePos, child, pos, p->tokens->at(item.pos));
					}

					if (result->indent) {
						if (item.pos == p->indentStart)
							result->indent->start = nodePos;
						else if (item.pos == p->indentEnd)
							result->indent->end = nodePos;
					}

					pos = child.pos();
				}

				assert(nodePos == 0, L"Node length computation was inaccurate!");
				return result;
			}

			void Parser::infoSubtree(InfoInternal *result, Nat &resultPos, TreeNode &startNode, Nat endPos) const {
				TreeNode *node = &startNode;
				Production *p = syntax->production(node->production());
				Item item = last(node->production());
				Nat pos = endPos;

				TreeArray children = node->children();

				// This production may be incomplete...
				while (item.tokenPos(p) > children.count())
					item.prev(p);

				Nat i = children.count();
				while (i > 0) {
					Nat childId = children[i-1];
					if (!item.prev(syntax))
						break;

					TreeNode &child = store->at(childId);
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
						infoToken(result, resultPos, child, pos, p->tokens->at(item.pos));

						pos = child.pos();
						i--;
					}
				}
			}

			void Parser::infoToken(InfoInternal *result, Nat &resultPos, TreeNode &node, Nat endPos, Token *token) const {
				assert(resultPos > 0, L"Too few children allocated in InfoNode!");

				InfoNode *created = null;
				if (node.children()) {
					if (Syntax::specialProd(node.production()) == Syntax::prodESkip) {
						// This is always an empty string! Even though it is a production, it should
						// look like a plain string.
						created = new (this) InfoLeaf(as<RegexToken>(token), emptyStr);
					} else {
						created = infoTree(node, endPos);
					}
				} else {
					Str *str = emptyStr;
					if (node.pos() < endPos) {
						Str::Iter from = source->posIter(node.pos());
						Str::Iter to = source->posIter(endPos);
						str = source->substr(from, to);
					}
					created = new (this) InfoLeaf(as<RegexToken>(token), str);
				}

				created->color = token->color;
				result->set(--resultPos, created);
			}

			Nat Parser::totalLength(TreeNode &node) const {
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

					TreeNode &child = store->at(childId);
					if (item.pos == Item::specialPos) {
						// Ignore this non-terminal.
						length--;

						// Traverse until we reach an epsilon production!
						for (TreeNode *at = &child;
							 at->children() && Syntax::specialProd(at->production()) == Syntax::prodRepeat;
							 at = &store->at(at->children()[0])) {

							Item i(syntax, at->production());
							length += at->children().count();
							if (i.pos == Item::specialPos)
								length -= 1;
						}
					}
				}

				return length;
			}

			ParseResult Parser::fullInfoTree() {
				if (matchEnd() == source->end())
					return ParseResult(infoTree(), 0, 0);

				// TODO: Cache the results of this computation in case fullInfoTree is called more
				// than once?

				// Save state.
				StackItem *acceptingStack = this->acceptingStack;
				Set<StackItem *> *lastSet = this->lastSet;
				Nat lastPos = this->lastPos;
				visited = new (this) BoolSet();
				stacks = new (this) FutureStacks();
				this->acceptingStack = null;
				this->lastSet = null;
				this->lastPos = 0;

				// Try to complete as many states as possible in order to arrive at a viable prefix.
				stacks->set(0, new (this) Set<StackItem *>(*lastSet));
				this->currentPos = lastPos;
				completePrefix();

				InfoNode *result = null;
				if (this->acceptingStack)
					result = infoTree(store->at(this->acceptingStack->tree), this->acceptingStack->pos);

				Nat corrections = stacks->top()->count() - lastSet->count();

				// Restore state.
				visited = null;
				stacks = null;
				this->acceptingStack = acceptingStack;
				this->lastSet = lastSet;
				this->lastPos = lastPos;

				// See if we need to add some more content...
				Nat skipped = 0;
				Nat totalLength = source->peekLength() - startPos;
				if (result && result->length() < totalLength) {
					skipped = totalLength - result->length();
					if (InfoInternal *node = as<InfoInternal>(result)) {
						Str::Iter from = source->posIter(startPos + result->length());
						InfoLeaf *add = new (this) InfoLeaf(null, source->substr(from, source->end()));

						InfoInternal *r = new (this) InfoInternal(node, node->count() + 1);
						r->set(node->count(), add);
						result = r;
					} else {
						// Concatenate everything into one large leaf node!
						result = null;
					}
				}

				// It is possible that we fail. If so: create a new node containing the entire desired range.
				if (!result) {
					Str::Iter from = source->posIter(startPos);
					result = new (this) InfoLeaf(null, source->substr(from, source->end()));
					if (skipped == 0)
						skipped = result->length();
				}

				return ParseResult(result, skipped, corrections);
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
				return store->count();
			}

			Nat Parser::byteCount() const {
				return store->byteCount();
			}

			void Parser::clearSyntax() {
				if (!table->empty())
					table = new (this) Table(syntax);
			}

		}
	}
}
