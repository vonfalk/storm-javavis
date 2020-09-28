#include "stdafx.h"
#include "Parser.h"
#include "Compiler/Lib/Array.h"

namespace storm {
	namespace syntax {
		namespace gll {

			Parser::Parser() {
				treeArrayType = StormInfo<TreePart>::handle(engine()).gcArrayType;

				rules = new (this) Array<RuleInfo *>();
				ruleId = new (this) Map<Rule *, Nat>();
				productions = new (this) Array<Production *>();
				productionId = new (this) Map<Production *, Nat>();
				currentStacks = new (this) Array<StackItem *>();
				syntaxPrepared = false;

				clear();
			}

			void Parser::add(Rule *rule) {
				Nat id = ruleId->get(rule, rules->count());
				if (id >= rules->count()) {
					ruleId->put(rule, rules->count());
					rules->push(new (this) RuleInfo(rule));
				}
			}

			void Parser::add(ProductionType *prod) {
				Production *p = prod->production;

				// No duplicates.
				if (productionId->has(p))
					return;

				productionId->put(p, productions->count());
				productions->push(p);

				// Add it to the rules for sorting and lookup.
				Rule *rule = prod->rule();
				Nat id = ruleId->get(rule, rules->count());
				if (id >= rules->count()) {
					ruleId->put(rule, rules->count());
					rules->push(new (this) RuleInfo(rule));
				}

				RuleInfo *info = rules->at(id);
				info->add(p);

				// Remember that we need to sort the data before parsing.
				syntaxPrepared = false;
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
				// var x = spawn foo
				prepare(str, file);
				PVAR(str);

				RuleInfo *rule = findRule(root);
				if (!rule)
					return false;

				matchFirst = start.offset();
				matchLast = 0;

				PLN(L"Parsing...");
				parse(rule, start.offset());

				return matchTree != null;
			}

			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx) {
				prepare(str, file);
				return InfoErrors();
			}

			void Parser::parse(RuleInfo *rule, Nat pos) {
				// Add all possible starting points as if we found this nonterminal in a production.
				// Note: We cannot use 'pqPushFirst' here, as it assumes 'prev' is non-null.
				for (Nat i = 0; i < rule->productions->count(); i++) {
					Production *p = rule->productions->at(i);
					pqPush(new (this) StackItem(true, null, p->firstLong(), pos));
					if (p->firstShort().valid())
						pqPush(new (this) StackItem(true, null, p->firstShort(), pos));
				}

				// Make sure "currentStacks" is large enough.
				currentStacks = new (this) Array<StackItem *>(productions->count() * 2, null);

				// Execute things on the stack until we're done.
				currentPos = pos;
				while (StackItem *top = pqPop()) {
					if (currentPos != top->inputPos()) {
						currentPos = top->inputPos();
						// We arrived at a new point in the input, clear the table of what we have parsed so far!
						for (Nat i = 0; i < currentStacks->count(); i++)
							currentStacks->at(i) = null;

						PLN(L"New position: " << currentPos);
					}

					parseStack(top);
				}
			}

			void Parser::parseStack(StackItem *top) {
				PLN(L"Parse " << top->inputPos() << L", " << top->depth() << L": " << top->iter);

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

				// 	StackItem *next = create(top, top->iter.nextLong(), matched, false);

				// 	// Save the short for later, if it is present.
				// 	pqPush(top, top->iter.nextShort(), matched, false);

				// 	// Advance.
				// 	top = next;
				// }
				// // Now, we know that "token" is not a regex.

				Token *token = top->iter.token();
				if (!token) {
					// At the end.
					// TODO: Remember that we popped this one!
					parseReduce(top);
				} else if (RuleToken *rule = token->asRule()) {
					// Rule. Create new stack tops on the PQ.
					RuleInfo *info = findRule(rule->rule);
					for (Nat i = 0; i < info->productions->count(); i++) {
						Production *p = info->productions->at(i);
						pqPushFirst(top, p->firstLong(), false);
						pqPushFirst(top, p->firstShort(), true);
					}
				} else if (RegexToken *regex = token->asRegex()) {
					Nat matched = regex->regex.matchRaw(src, top->inputPos());
					if (matched == Regex::NO_MATCH) {
						// Nothing to do.
						return;
					}

					pqPush(top, top->iter.nextLong(), matched);
					pqPush(top, top->iter.nextShort(), matched);
				}
			}

			void Parser::parseReduce(StackItem *top) {
				Nat inputPos = top->inputPos();
				PLN(L"Reducing at " << inputPos << L": " << top->iter);

				// Find the length of this branch, and the new top.
				// Note: We only fork when "first" is true.
				StackItem *newTop;
				Nat count = 0;
				for (newTop = top; !newTop->first(); newTop = newTop->prev(0))
					count++;

				// We need to skip one more, but this might actually fork! So we do this later.
				// If we can bail out early from knowing that we won't need it already, we do that now.
				if (newTop->prevCount() == 0) {
					if (!updateMatch(matchTree, matchLast, top->iter.production(), inputPos))
						return;
				}

				// Create a Tree-array for this branch.
				GcArray<TreePart> *match = runtime::allocArray<TreePart>(engine(), treeArrayType, count);
				match->filled = productionId->get(top->iter.production());

				StackItem *at = top;
				for (Nat i = count; i > 0; i--) {
					at = at->prev(0);
					if (at->match) {
						match->v[i - 1] = TreePart(at->match, at->inputPos());
					} else {
						match->v[i - 1] = TreePart(at->inputPos());
					}
				}

				// Are we done?
				if (newTop->prevCount() == 0) {
					// Done!
					PLN(L"Done at " << inputPos);
					matchTree = match;
					matchLast = inputPos;
					return;
				}

				// Advance to "all" previous states.
				count = newTop->prevCount();
				for (Nat i = 0; i < count; i++) {
					top = newTop->prev(i);

					// Keep higher priority states.
					if (!updateMatch(top->match, top->inputPos(), newTop->iter.production(), inputPos))
						continue;

					PLN(L" => into " << top->iter << L" (" << top << L")");

					// Save the match.
					top->match = match;

					// Add new states.
					pqPush(top, top->iter.nextLong(), inputPos);
					pqPush(top, top->iter.nextShort(), inputPos);
				}
			}

			bool Parser::updateMatch(Tree *prev, Nat prevPos, Production *current, Nat currentPos) {
				// If no previous match, or if the previous match was shorter (we're always greedy), update!
				if (!prev || prevPos < currentPos)
					return true;

				// If we're at the same position, then we need to worry about priorities.
				Production *p = productions->at(prev->filled);
				return p->priority < current->priority;
			}

			struct PQCompare {
				bool operator()(const StackItem *a, const StackItem *b) const {
					// Smallest first!
					return *b < *a;
				}
			};

			void Parser::pqInit() {
				const size_t initialSize = 512;
				pq = runtime::allocArray<StackItem *>(engine(), &pointerArrayType, initialSize);
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
				if (iter.valid())
					pqPush(create(prev, iter, inputPos));
			}

			void Parser::pqPushFirst(StackItem *prev, ProductionIter iter, Bool second) {
				if (!iter.valid())
					return;

				// Check if this is already present, if so: merge it with the previous one at this location.
				Nat id = productionId->get(iter.production())*2 + Nat(second);
				StackItem *&duplicate = currentStacks->at(id);
				if (duplicate) {
					PLN(L"Duplicate of " << iter);
					duplicate->prevPush(prev);
					// TODO: When we do have epsilon productions, we might need to reduce this immediately.
				} else {
					PLN(L"Push first " << iter);
					duplicate = new (this) StackItem(true, prev, iter, prev->inputPos());
					pqPush(duplicate);
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

			StackItem *Parser::create(StackItem *prev, ProductionIter iter, Nat inputPos) {
				PLN(L"Created state " << inputPos << L", " << iter);
				return new (this) StackItem(false, prev, iter, inputPos);
			}

			void Parser::clear() {
				url = null;
				src = null;
				pq = null;
				matchTree = null;
				matchFirst = matchLast = 0;
			}

			Bool Parser::hasError() const {
				return matchTree == null || matchLast != src->peekLength();
			}

			Bool Parser::hasTree() const {
				return matchTree != null;
			}

			Str::Iter Parser::matchEnd() const {
				if (src)
					return src->posIter(matchLast);
				else
					return Str::Iter();
			}

			Str *Parser::errorMsg() const {
				return new (this) Str(S("TODO!"));
			}

			SrcPos Parser::errorPos() const {
				return SrcPos();
			}

			Node *Parser::tree() const {
				Node *n = tree(matchTree, matchFirst, matchLast);
				PVAR(n);
				return n;
			}

			InfoNode *Parser::infoTree() const {
				if (!hasTree())
					return null;

				return infoTree(matchTree, matchLast);
			}

			Nat Parser::stateCount() const {
				return 0;
			}

			Nat Parser::byteCount() const {
				return 0;
			}

			void Parser::prepare(Str *str, Url *file) {
				src = str;
				url = file;
				stackId = 0;
				matchTree = null;
				matchLast = 0;

				pqInit();

				// Need to pre-process the grammar?
				if (syntaxPrepared)
					return;

				for (Nat i = 0; i < rules->count(); i++) {
					RuleInfo *info = rules->at(i);
					info->sort();
				}

				syntaxPrepared = true;
			}

			MAYBE(RuleInfo *) Parser::findRule(Rule *rule) const {
				Nat id = ruleId->get(rule, rules->count());
				if (id < rules->count())
					return rules->at(id);
				else
					return null;
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
