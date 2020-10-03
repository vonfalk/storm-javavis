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
				currentStacks = new (this) Array<StackFirst *>();
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
				// Make sure "currentStacks" is large enough.
				currentStacks = new (this) Array<StackFirst *>(productions->count(), null);

				// Add all possible starting points as if we found this nonterminal in a production.
				// Note: We cannot use 'pqPushFirst' here, as it assumes 'prev' is non-null.
				for (Nat i = 0; i < rule->productions->count(); i++) {
					Production *p = rule->productions->at(i);
					StackFirst *created = new (this) StackFirst(null, p, pos);
					pqPush(created);
					currentStacks->at(productionId->get(p)) = created;
				}

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
				if (!top)
					return;

				PLN(L"Parse " << top);

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
					// Simply emit two new states. We don't even have to put them in the queue.
					parseStack(create(first, first->nextLong(), first->inputPos()));
					parseStack(create(first, first->nextShort(), first->inputPos()));

				} else if (StackRule *rule = top->asRule()) {
					// It is a rule match. Create a new stack top for each production.
					RuleInfo *info = findRule(rule->iter.token()->asRule()->rule);
					for (Nat i = 0; i < info->productions->count(); i++) {
						Production *p = info->productions->at(i);
						pqPushFirst(rule, p);
					}

				} else if (StackMatch *match = top->asMatch()) {
					// It is a regex match (since Rule was before). Try to match it.
					Nat matched = match->iter.token()->asRegex()->regex.matchRaw(src, top->inputPos());
					if (matched == Regex::NO_MATCH) {
						return;
					}

					pqPush(top, match->iter.nextLong(), matched);
					pqPush(top, match->iter.nextShort(), matched);

				} else {
					// None of the above, it must be the end of the production.
					parseReduce(top);
				}
			}

			void Parser::parseReduce(StackItem *top) {
				Nat matchEnd = top->inputPos();

				// Find the first element and the length of this branch. Note: We skip 'top', as we
				// know that it is the "end" placeholder.
				StackFirst *first = null;
				Nat count = 0;
				for (StackItem *at = top->prev; !(first = at->asFirst()); at = at->prev)
					count++;

				PLN(L"At " << matchEnd << L", reducing " << first->production);

				// If we already have a match at this location, we consider them to be equal.
				// Note: this is only representing a single production, so we don't have to
				// worry about priorities at all.
				if (first->matchEnd == matchEnd && first->match)
					return;

				// Create a Tree-array for this branch.
				GcArray<TreePart> *match = runtime::allocArray<TreePart>(engine(), treeArrayType, count);
				match->filled = productionId->get(first->production);

				// Extract the elements.
				for (StackItem *at = top->prev; count > 0; at = at->prev, count--) {
					match->v[count - 1] = TreePart(at->match(), at->inputPos());
				}

				// Store the match for future occurrences.
				first->match = match;
				first->matchEnd = matchEnd;

				// Update all previous nodes with the new, potentially better, match.
				count = first->prevCount();
				for (Nat i = 0; i < count; i++) {
					StackRule *prev = first->prevAt(i);
					if (!prev) {
						// We're done!
						PLN(L"Done at " << matchEnd);
						if (updateMatch(matchTree, matchLast, first->production, matchEnd)) {
							matchTree = match;
							matchLast = matchEnd;
						}
					} else {
						// Update this node if we have a better match.
						advanceRule(prev, first);
					}
				}
			}

			void Parser::advanceRule(StackRule *advance, StackFirst *match) {
				// TODO: We might need to recurse a bit if this node is "completed"?
				// This would only be an issue where we have a lot of epsilon productions,
				// however, so maybe we don't care?

				Nat matchEnd = match->matchEnd;
				if (updateMatch(advance->match, advance->matchEnd, match->production, matchEnd)) {
					// Make sure not to create duplicate "future" states. We only create one set of
					// state for each unique "end" position we find.
					if (!advance->match || advance->matchEnd != matchEnd) {
						pqPush(advance, advance->iter.nextLong(), match->matchEnd);
						pqPush(advance, advance->iter.nextShort(), match->matchEnd);
					} else {
						// If we did not create new states, this production might have been
						// completed already, and thus the 'match' at the start might contain stale
						// information. Go back and update it!
						updateTreeMatch(advance, match->match);
					}

					advance->match = match->match;
					advance->matchEnd = matchEnd;
				}
			}

			void Parser::updateTreeMatch(StackRule *update, GcArray<TreePart> *newMatch) {
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

			void Parser::pqPush(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos) {
				if (StackItem *created = create(prev, iter, inputPos)) {
					PLN(L"Push " << created);
					pqPush(created);
				}
			}

			void Parser::pqPushFirst(StackRule *prev, Production *production) {
				// Check if this is already present, if so: merge it with the previous one at this location.
				StackFirst *&duplicate = currentStacks->at(productionId->get(production));
				if (duplicate) {
					PLN(L"Duplicate of " << production);
					duplicate->prevPush(prev);
					if (prev && duplicate->match) {
						// If there is already a match, then we can "reduce" this one immediately.
						// Note: We don't have to worry about getting the start state "late", as those
						// are always pushed as the first thing in the parse.
						advanceRule(prev, duplicate);
					}
				} else {
					PLN(L"Push first " << production);
					duplicate = new (this) StackFirst(prev, production, prev->inputPos());
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

			StackItem *Parser::create(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos) {
				if (!iter.valid())
					return null;

				Token *token = iter.token();
				if (!token)
					return new (this) StackItem(prev, inputPos);
				else if (token->asRegex())
					return new (this) StackMatch(prev, iter, inputPos);
				else if (token->asRule())
					return new (this) StackRule(prev, iter, inputPos);

				assert(false, L"Unknown token type!");
				return null;
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
