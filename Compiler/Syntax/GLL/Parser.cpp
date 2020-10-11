#include "stdafx.h"
#include "Parser.h"
#include "Compiler/Lib/Array.h"
#include "Compiler/Engine.h"

namespace storm {
	namespace syntax {
		namespace gll {

			/**
			 * Some knobs to tweak the parser's performance:
			 */

			// Initial size of the priority queue. This is quite a lot of headroom. In normal code,
			// about 40 elements are required.
			static const size_t pqCount = 256;

			// Initial size of the "last stacks" list. Should be large enough to store all stack
			// tops generated at a particular location. This is a lot of headroom. In normal code
			// about 30-40 elements are required.
			static const size_t historyCount = 128;


			Parser::Parser() {
				treeArrayType = StormInfo<TreePart>::handle(engine()).gcArrayType;

				rules = new (this) Array<RuleInfo *>();
				ruleId = new (this) Map<Rule *, Nat>();
				productions = new (this) Array<Production *>();
				productionId = new (this) Map<Production *, Nat>();
				currentStacks = null;
				parentReqCount = 0;

				clear();
			}

			MAYBE(RuleInfo *) Parser::findRule(Rule *rule) const {
				Nat id = ruleId->get(rule, rules->count());
				if (id < rules->count())
					return rules->at(id);
				else
					return null;
			}

			RuleInfo *Parser::findAddRule(Rule *rule) {
				Nat id = ruleId->get(rule, rules->count());
				if (id >= rules->count()) {
					RuleInfo *result = new (this) RuleInfo(rule);
					ruleId->put(rule, rules->count());
					rules->push(result);
					return result;
				} else {
					return rules->at(id);
				}
			}

			void Parser::add(Rule *rule) {
				findAddRule(rule);
			}

			void Parser::add(ProductionType *prod) {
				Production *p = prod->production;

				// No duplicates.
				if (productionId->has(p))
					return;

				productionId->put(p, productions->count());
				productions->push(p);

				// Add it to the rules for lookup.
				RuleInfo *ruleInfo = findAddRule(prod->rule());
				ruleInfo->add(p);

				// Check its parent, if any, and ensure that the parent is allocated a parent ID.
				if (p->parent) {
					RuleInfo *parent = findAddRule(p->parent);
					if (parent->requirement.empty()) {
						parent->requirement = ParentReq(engine(), parentReqCount++);
					}
				}
			}

			Bool Parser::sameSyntax(ParserBackend *other) {
				if (runtime::typeOf(this) != runtime::typeOf(other))
					return false;
				Parser *o = (Parser *)other;

				// Note: We're a bit picky about having the exact same rules, even if they happen to
				// be empty in one place and non-existent in the other.
				if (rules->count() != o->rules->count())
					return false;

				for (Nat i = 0; i < rules->count(); i++) {
					Nat id = o->ruleId->get(rules->at(i)->rule, o->rules->count());
					if (id >= o->rules->count())
						return false;

					if (!rules->at(i)->sameSyntax(o->rules->at(i)))
						return false;
				}

				return true;
			}

			Bool Parser::parse(Rule *root, Str *str, Url *file, Str::Iter start) {
				prepare(str, file);
				// PVAR(str);

				RuleInfo *rule = findRule(root);
				if (!rule)
					return false;

				parse(rule, ParentReq(), start.offset());

				// Clear the priority queue to save memory.
				pq = null;

				return result != null && result->match != null;
			}

			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx) {
				prepare(str, file);

				RuleInfo *rule = findRule(root);
				if (!rule)
					return infoFailure();

				// Create a suitable context.
				ParentReq context;
				for (InfoInternal *at = ctx; at; at = at->parent()) {
					RuleInfo *r = findRule(at->production()->rule());
					if (r) {
						context = context.concat(engine(), r->requirement);
					}
				}

				parse(rule, context, start.offset());

				// Clear the priority queue to save memory.
				pq = null;

				if (result != null && result->match != null) {
					return infoSuccess();
				} else {
					return infoFailure();
				}
			}

			void Parser::prepare(Str *str, Url *file) {
				src = str;
				url = file;
				result = null;

				pqInit();

				// Make sure "currentStacks" is large enough.
				currentStacks = runtime::allocArray<StackFirst *>(engine(), &pointerArrayType, rules->count());

				// Create some room for storing stack tops.
				lastStacks = runtime::allocArray<StackMatch *>(engine(), &pointerArrayType, historyCount);
			}

			void Parser::parse(RuleInfo *rule, ParentReq context, Nat pos) {
				// Add a starting state.
				// Note: We cannot use 'pqPushFirst' here, as it assumes 'prev' is non-null.
				result = new (this) StackFirst(null, context, rule, pos);
				pqPush(result);
				currentStacks->v[ruleId->get(rule->rule)] = result;

				// Execute things on the stack until we're done.
				currentPos = pos;
				while (StackItem *top = pqPop()) {
					if (currentPos != top->inputPos()) {
						currentPos = top->inputPos();
						// We arrived at a new point in the input, clear the table of what we have parsed so far!
						memset(currentStacks->v, 0, sizeof(StackFirst *) * currentStacks->count);
						lastClear();

						// PLN(L"New position: " << currentPos);
					}

					parseStack(top);
				}
			}

			void Parser::lastPush(StackMatch *item) {
				if (lastStacks->filled >= lastStacks->count) {
					// Grow it. This should be unlikely.
					GcArray<StackMatch *> *n;
					n = runtime::allocArray<StackMatch *>(engine(), &pointerArrayType, lastStacks->count * 2);
					memcpy(n->v, lastStacks->v, sizeof(StackItem *) * lastStacks->count);
					n->filled = lastStacks->filled;
					lastStacks = n;
				}

				lastStacks->v[lastStacks->filled++] = item;
			}

			void Parser::lastClear() {
				// Don't memset more than needed. We could skip doing it, but that will cause us to
				// retain objects that we might not need, which makes the GC's job more difficult.
				memset(lastStacks->v, 0, lastStacks->filled * sizeof(StackItem *));
				lastStacks->filled = 0;
			}

			void Parser::parseStack(StackItem *top) {
				if (!top)
					return;

				// PLN(L"Parse " << top);

				// This is a nice optimization, but it makes it a lot harder to reason about when
				// things happen. Currently, we require that new rules are instantiated at the
				// "current location", which means that we cannot be too far ahead. Thus, we might
				// want to evaluate if this optimization is worth doing at a later stage (I don't
				// think so, as it will only be beneficial when we have two terminals next to each
				// other, something that does not happen often due to the "," token).

				// Token *token;
				// RegexToken *regex;
				// while ((token = top->iter.token()) && (regex = token->asRegex())) {
				// 	// Try to step multiple regex steps without going through the PQ, so that we
				// 	// don't have to use the PQ for these states, reducing the required time.
				// 	Nat matched = regex->regex.matchRaw(src, top->inputPos());
				// 	if (matched == Regex::NO_MATCH) {
				// 		// Nothing to do! Kill of this branch entirely.
				// 		return;
				// 	}

				// 	StackItem *next = new (this) StackItem(top, top->iter.nextLong(), matched);

				// 	// Save the short for later, if it is present.
				// 	pqPush(top, top->iter.nextShort(), matched, false);

				// 	// Advance.
				// 	top = next;
				// }
				// // Now, we know that "token" is not a regex.

				if (StackFirst *first = top->asFirst()) {
					Engine &e = engine();

					// Emit states for each of the productions in this rule.
					RuleInfo *info = first->rule;

					// Note 'concat' is basically no-op if either of the ParentReq objects are empty.
					ParentReq requirements = first->requirements.concat(e, info->requirement);

					for (Nat i = 0; i < info->productions->count(); i++) {
						Production *p = info->productions->at(i);
						ParentReq req = requirements;

						if (p->parent) {
							RuleInfo *info = findRule(p->parent);
							// Note: We're comparing to the unmodified requirements, thus a
							// production can not fullfill its own requirement.
							if (!first->requirements.has(info->requirement))
								continue;

							// Note: We might need to add ourselves back, as we might depend on ourself.
							req = req.remove(e, findRule(p->parent)->requirement);
							req = req.concat(e, info->requirement);
						}

						parseStack(create(first, p->firstLong(), first->inputPos(), req));
						parseStack(create(first, p->firstShort(), first->inputPos(), req));
					}

				} else if (StackRule *rule = top->asRule()) {
					lastPush(rule);
					// It is a rule match. Create a new stack top for the rule. This might be
					// de-duplicated, so we can't recurse immediately here.
					pqPushFirst(rule, rule->iter.token()->asRule()->rule);

				} else if (StackMatch *match = top->asMatch()) {
					lastPush(match);
					// It is a regex match (since Rule was before). Try to match it.
					Nat matched = match->iter.token()->asRegex()->regex.matchRaw(src, top->inputPos());
					if (matched == Regex::NO_MATCH) {
						return;
					}

					pqPush(top, match->iter.nextLong(), matched);
					pqPush(top, match->iter.nextShort(), matched);

				} else if (StackEnd *end = top->asEnd()) {
					// End of a production.
					parseReduce(end);
				}
			}

			void Parser::parseReduce(StackEnd *top) {
				Nat matchEnd = top->inputPos();

				// Find the first element and the length of this branch. Note: We skip 'top', as we
				// know that it is the "end" placeholder.
				StackFirst *first = null;
				Nat count = 0;
				for (StackItem *at = top->prev; !(first = at->asFirst()); at = at->prev)
					count++;

				// PLN(L"At " << matchEnd << L", (" << (void *)first <<  L") reducing " << top->production);

				// What shall we do with this match? There are three major cases:
				// 1: This is the first match, then we shall simply accept it.
				// 2: This is not the first match, and we're at the same input position. Check priorities.
				// 3: This is not the first match, and the input position differs. Duplicate the previous states.

				Int oldPrio = 0;
				Int newPrio = 0;
				if (first->match && first->matchEnd == matchEnd) {
					// Check priority for case 2, so that we can do an early out and avoid
					// allocations if we don't need the tree.
					oldPrio = productions->at(first->match->filled)->priority;
					newPrio = top->production->priority;

					// If they are equal, we need to recursively compare the trees. It is easier to
					// do that when we have generated the array - otherwise we would need a special
					// case for the first recursion level.
					if (oldPrio > newPrio)
						return;
				}

				// Create a Tree-array for this branch.
				Tree *match = runtime::allocArray<TreePart>(engine(), treeArrayType, count);
				match->filled = productionId->get(top->production);

				// Extract the elements.
				for (StackItem *at = top->prev; count > 0; at = at->prev, count--) {
					match->v[count - 1] = TreePart(at->match(), at->inputPos());
				}

				// Store and/or update the match.
				count = first->prevCount();
				if (!first->match) {
					// Case #1 - no prior match.
					for (Nat i = 0; i < count; i++) {
						StackRule *prev = first->prevAt(i);

						prev->match = match;

						pqPush(prev, prev->iter.nextLong(), matchEnd);
						pqPush(prev, prev->iter.nextShort(), matchEnd);
					}
				} else if (first->matchEnd == matchEnd) {
					// Case #2 - update previous match (we checked priorities earlier).

					// If they were the same, we need to compare deeper.
					if (oldPrio == newPrio)
						if (compare(first->match, match) >= 0)
							return;

					for (Nat i = 0; i < count; i++) {
						StackRule *prev = first->prevAt(i);

						// Update in the previously generated Tree-array.
						updateTreeMatch(prev, match);
						prev->match = match;
					}
				} else {
					// Case #3 - duplicate the 'prev' state and go on, this is a new match at another position.
					for (Nat i = 0; i < count; i++) {
						StackRule *prev = first->prevAt(i);

						// Duplicate the state, and continue that chain.
						StackRule *r = new (this) StackRule(*prev);
						r->match = match;

						pqPush(r, r->iter.nextLong(), matchEnd);
						pqPush(r, r->iter.nextShort(), matchEnd);

						// Update the list of previous states.
						first->prevAt(i, r);
					}
				}

				// Remember what we stored for future potentiall reductions.
				first->match = match;
				first->matchEnd = matchEnd;
			}

			void Parser::updateTreeMatch(StackRule *update, Tree *newMatch) {
				StackFirst *first = null;
				Nat index = 0;
				for (StackItem *at = update->prev; !(first = at->asFirst()); at = at->prev)
					index++;

				// Bail out if there is nothing to modify.
				if (!first->match || first->match->count <= index)
					return;

				// Only update it if the correct element is where we expect it to be. Otherwise,
				// the current match might correspond to another match that 'update' is not
				// involved in.
				//
				// Checking for match equivalence should be enough here - I cannot imagine any cases
				// where a different match happens to use the same part of a match. We could add
				// pointers to the end of matched productions to validate this properly, but this
				// would mean that the GC will be unable to reclaim a large number of states that
				// represent the "current best match", which will probably impact performance.
				TreePart &part = first->match->v[index];
				if (part.match == update->match)
					part.match = newMatch;
			}

			Int Parser::compare(Tree *lhs, Tree *rhs) {
				// If one of them is null, then we prefer the non-null one.
				if (!lhs && !rhs)
					return 0;
				if (!lhs)
					return 1;
				if (!rhs)
					return -1;

				// Comparing different productions will compare their priorities.
				if (lhs->filled != rhs->filled) {
					Production *l = productions->at(lhs->filled);
					Production *r = productions->at(rhs->filled);
					if (l->priority < r->priority)
						return -1;
					else if (l->priority > r->priority)
						return 1;
					else
						return 0;
				}

				// Same production. Do a lexiographic comparison.
				Nat count = min(lhs->count, rhs->count);
				for (Nat i = 0; i < count; i++) {
					Int r = compare(lhs->v[i].match, rhs->v[i].match);
					if (r != 0)
						return r;
				}

				// If we get here, we just pick the longest one. This makes * and + greedy.
				if (lhs->count < rhs->count)
					return -1;
				else if (lhs->count > rhs->count)
					return 1;
				else
					return 0;
			}

			struct PQCompare {
				bool operator()(const StackItem *a, const StackItem *b) const {
					// Smallest first!
					return *b < *a;
				}
			};

			void Parser::pqInit() {
				pq = runtime::allocArray<StackItem *>(engine(), &pointerArrayType, pqCount);
				pq->filled = 0;
			}

			void Parser::pqPush(StackItem *item) {
				if (pq->filled >= pq->count) {
					GcArray<StackItem *> *n = runtime::allocArray<StackItem *>(engine(), &pointerArrayType, pq->count * 2);
					memcpy(n->v, pq->v, pq->filled * sizeof(StackItem *));
					n->filled = pq->filled;
					pq = n;
				}

				pq->v[pq->filled++] = item;
				std::push_heap(pq->v, pq->v + pq->filled, PQCompare());
			}

			void Parser::pqPush(StackItem *prev, ProductionIter iter, Nat inputPos) {
				if (StackItem *created = create(prev, iter, inputPos, prev->requirements)) {
					// PLN(L"Push " << created);
					pqPush(created);
				}
			}

			void Parser::pqPushFirst(StackRule *prev, Rule *rule) {
				// Check if this is already present, if so: merge it with the previous one at this location.
				Nat id = ruleId->get(rule);
				StackFirst *duplicate = currentStacks->v[id];

				// Find the one with the same set of requirements as we have.
				while (duplicate && duplicate->requirements != prev->requirements) {
					duplicate = duplicate->nextDuplicate;
				}

				if (duplicate) {
					// PLN(L"Duplicate of " << rule);
					duplicate->prevPush(prev);
					if (prev && duplicate->match) {
						// If there is already a match, then we can "reduce" this one immediately.
						// Note: We don't have to worry about getting the start state "late", as those
						// are always pushed as the first thing in the parse.
						prev->match = duplicate->match;

						pqPush(prev, prev->iter.nextLong(), duplicate->matchEnd);
						pqPush(prev, prev->iter.nextShort(), duplicate->matchEnd);
					}
				} else {
					// PLN(L"Push first " << rule);
					StackFirst *created = new (this) StackFirst(prev, prev->requirements, rules->at(id), prev->inputPos());
					pqPush(created);

					// Put it into the linked list.
					created->nextDuplicate = currentStacks->v[id];
					currentStacks->v[id] = created;
				}
			}

			StackItem *Parser::pqPop() {
				if (pq->filled == 0)
					return null;

				std::pop_heap(pq->v, pq->v + pq->filled, PQCompare());
				StackItem *r = pq->v[--pq->filled];
				pq->v[pq->filled] = null;
				return r;
			}

			StackItem *Parser::create(StackItem *prev, ProductionIter iter, Nat inputPos, const ParentReq &req) {
				if (!iter.valid())
					return null;

				Token *token = iter.token();
				if (!token)
					return new (this) StackEnd(prev, req, iter.production(), inputPos);
				else if (token->asRegex())
					return new (this) StackMatch(prev, req, iter, inputPos);
				else if (token->asRule())
					return new (this) StackRule(prev, req, iter, inputPos);

				assert(false, L"Unknown token type!");
				return null;
			}

			void Parser::clear() {
				url = null;
				src = null;
				pq = null;
				result = null;
				lastStacks = null;
			}

			Bool Parser::hasError() const {
				if (!lastStacks || lastStacks->filled == 0)
					return false;

				return result == null
					|| result->matchEnd != src->peekLength();
			}

			Bool Parser::hasTree() const {
				return result != null
					&& result->match != null;
			}

			Str::Iter Parser::matchEnd() const {
				if (src && result)
					return src->posIter(result->matchEnd);
				else
					return Str::Iter();
			}

			Str *Parser::errorMsg() const {
				StrBuf *msg = new (this) StrBuf();
				if (currentPos == src->peekLength()) {
					*msg << S("Unexpected end of file.");
				} else {
					Str *ch = new (this) Str(src->posIter(currentPos).v());
					*msg << S("Unexpected '") << ch->escape() << S("'.");
				}

				Map<Str *, Str *> *errors = new (this) Map<Str *, Str *>();
				for (Nat i = 0; i < lastStacks->filled; i++) {
					StackMatch *match = lastStacks->v[i];
					if (!match->asRule()) {
						// Produce a message.
						errorMsg(errors, match, match->iter);
					}
				}

				if (errors->any()) {
					*msg << S(" Expected:");
					msg->indent();
					for (Map<Str *, Str *>::Iter i = errors->begin(); i != errors->end(); ++i) {
						*msg << S("\n") << i.k();
						msg->indent();
						*msg << i.v();
						msg->dedent();
					}
				}

				return msg->toS();
			}

			static Bool isBoring(ProductionIter iter) {
				Production *p = iter.production();
				if (!p)
					return true;

				if (iter == p->firstLong() || iter == p->firstShort())
					return p->tokens->count() < 5;
				else
					return p->tokens->count() < 2;
			}

			static void addContext(Str *&to, StackItem *item, Nat depth = 0) {
				if (StackMatch *match = item->asMatch()) {
					if (!isBoring(match->iter) || depth >= 5) {
						Str *line = TO_S(item, S("\nin: ") << match->iter);
						TODO(L"De-duplicate! Perhaps using a map.");
						//if (!strstr(to->c_str(), line->c_str()))
						to = *to + line;
						return;
					}
				}

				// Traverse until we find a StackFirst, and check all possible previous branches.
				while (item) {
					if (StackFirst *first = item->asFirst()) {
						for (Nat i = 0; i < first->prevCount(); i++) {
							addContext(to, first->prevAt(i), depth + 1);
						}
						return;
					}

					item = item->prev;
				}
			}

			void Parser::errorMsg(Map<Str *, Str *> *msg, StackItem *item, ProductionIter iter) const {
				if (!iter.valid())
					return;

				RegexToken *t = iter.token()->asRegex();
				if (!t)
					return;

				Str *key = TO_S(this, S("\"") << t->regex << S("\""));
				Str *val = new (this) Str();
				if (msg->has(key))
					val = msg->get(key);

				addContext(val, item);

				msg->put(key, val);
			}

			SrcPos Parser::errorPos() const {
				return SrcPos(url, currentPos, currentPos + 1);
			}

			Node *Parser::tree() const {
				Gc::RampAlloc z(engine().gc);
				Node *n = tree(result->match, result->inputPos(), result->matchEnd);
				// PVAR(n);
				return n;
			}

			InfoNode *Parser::infoTree() const {
				if (!hasTree())
					return null;

				Gc::RampAlloc z(engine().gc);
				return infoTree(result->match, result->matchEnd);
			}

			Nat Parser::stateCount() const {
				return 0;
			}

			Nat Parser::byteCount() const {
				return 0;
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

			void Parser::setNode(Node *into, Token *token, TreePart part, Nat last) const {
				if (!token->target)
					return;

				if (part.isRule()) {
					// Nonterminal.
					Node *t = tree(part.match, part.startPos, last);
					setValue(into, token->target, t);
				} else {
					// It is a terminal symbol. Extract the string.
					Str::Iter from = src->posIter(part.startPos);
					Str::Iter to = src->posIter(last);
					SrcPos pos(url, part.startPos, last);
					SStr *v =  new (this) SStr(src->substr(from, to), pos);
					setValue(into, token->target, v);
				}
			}

			Node *Parser::tree(Tree *root, Nat firstPos, Nat lastPos) const {
				Production *p = productions->at(root->filled);
				Nat repCount = p->repetitionsFor(root->count);
				Node *result = allocNode(p, firstPos, lastPos);

				Nat repStart = firstPos;
				Nat repEnd = lastPos;
				ProductionIter at = p->firstFixed(repCount);
				for (Nat i = 0; i < root->count; i++) {
					// Remember the captured part (if any). Note: This does not need to work with
					// repeating stuff, as that combination is not currently supported.
					if (at.position() == p->repStart)
						repStart = root->v[i].startPos;
					if (at.position() == p->repEnd)
						repEnd = root->v[i].startPos;

					Token *t = at.token();
					if (t && t->target) {
						Nat next = lastPos;
						if (i + 1 < root->count)
							next = root->v[i + 1].startPos;
						setNode(result, t, root->v[i], next);
					}

					at = at.nextFixed(repCount);
				}

				if (p->repCapture) {
					Str::Iter from = src->posIter(repStart);
					Str::Iter to = src->posIter(repEnd);
					SrcPos pos(url, repStart, repEnd);
					SStr *v = new (this) SStr(src->substr(from, to), pos);
					setValue(result, p->repCapture->target, v);
				}

				return result;
			}

			Node *Parser::allocNode(Production *from, Nat start, Nat end) const {
				ProductionType *type = from->type();

				// A bit ugly, but this is enough for the object to be considered a proper object
				// when it is populated.
				void *mem = runtime::allocObject(type->size().current(), type);
				Node *r = new (Place(mem)) Node(SrcPos(url, start, end));
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

			InfoNode *Parser::infoTree(Tree *root, Nat lastPos) const {
				Production *p = productions->at(root->filled);
				Nat repCount = p->repetitionsFor(root->count);
				InfoInternal *result = new (this) InfoInternal(p, root->count);

				ProductionIter at = p->firstFixed(repCount);
				for (Nat i = 0; i < root->count; i++) {
					Token *t = at.token();

					if (t) {
						Nat next = lastPos;
						if (i + 1 < root->count)
							next = root->v[i + 1].startPos;

						TreePart &part = root->v[i];
						InfoNode *created = null;
						if (part.isRule()) {
							created = infoTree(part.match, next);
						} else {
							Str::Iter start = src->posIter(part.startPos);
							Str::Iter end = src->posIter(next);
							created = new (this) InfoLeaf(t->asRegex(), src->substr(start, end));
						}
						created->color = t->color;
						result->set(i, created);
					}

					at = at.nextFixed(repCount);
				}

				return result;
			}
		}
	}
}
