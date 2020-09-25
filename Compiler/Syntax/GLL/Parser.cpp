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
				prodId = new (this) Map<Production *, Nat>();
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
				Rule *rule = prod->rule();
				Nat id = ruleId->get(rule, rules->count());
				if (id >= rules->count()) {
					ruleId->put(rule, rules->count());
					rules->push(new (this) RuleInfo(rule));
				}

				RuleInfo *info = rules->at(id);
				info->add(prod);

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
				prepare(str, file);
				PVAR(str);

				RuleInfo *rule = findRule(root);
				if (!rule)
					return false;

				matchRule = ruleId->get(root);
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
				for (Nat i = 0; i < rule->productions->count(); i++) {
					Production *p = rule->productions->at(i);
					pqPush(null, p->firstLong(), pos, true);
					pqPush(null, p->firstShort(), pos, true);
				}

				// Execute things on the stack until we're done.
				Nat lastPos = 0;
				while (StackItem *top = pqPop()) {
					if (lastPos != top->inputPos()) {
						lastPos = top->inputPos();
						// We arrived at a new point in the input, clear the table of what we have parsed so far!
					}

					// TODO: If we have already seen this StackItem, don't parse it again!

					parseStack(top);
				}
			}

			void Parser::parseStack(StackItem *top) {
				PLN(L"Parse " << top->inputPos() << L": " << top->iter);

				Token *token = top->iter.token();

				// Now, we know that "token" is not a regex.
				if (!token) {
					// At the end.
					// TODO: Remember that we popped this one!
					parseReduce(top);
				} else if (RuleToken *rule = token->asRule()) {
					// Rule. Create new stack tops on the PQ.
					RuleInfo *info = findRule(rule->rule);
					for (Nat i = 0; i < info->productions->count(); i++) {
						Production *p = info->productions->at(i);
						pqPush(top, p->firstLong(), top->inputPos(), true);
						pqPush(top, p->firstShort(), top->inputPos(), true);
					}
				} else if (RegexToken *regex = token->asRegex()) {
					// Regex.
					// TODO: Try to step multiple regex steps without going through the PQ?
					// This could, however, mess up the priorities.
					Nat matched = regex->regex.matchRaw(src, top->inputPos());
					if (matched == Regex::NO_MATCH) {
						// Nothing to do! Kill of this branch entirely.
						return;
					}

					pqPush(top, top->iter.nextLong(), matched, false);
					pqPush(top, top->iter.nextShort(), matched, false);
				}
			}

			void Parser::parseReduce(StackItem *top) {
				Nat inputPos = top->inputPos();
				PLN(L"Reducing at " << inputPos << L": " << top->iter);

				// Find the length of this branch.
				Nat count = 0;
				for (StackItem *at = top; !at->first(); at = at->prev)
					count++;

				// Create a Tree-array for this branch.
				GcArray<TreePart> *match = runtime::allocArray<TreePart>(engine(), treeArrayType, count);
				match->filled = prodId->get(top->iter.production());

				for (Nat i = count; i > 0; i--) {
					top = top->prev;
					if (top->match) {
						match->v[i - 1] = TreePart(top->match, top->inputPos());
					} else {
						match->v[i - 1] = TreePart(top->inputPos());
					}
				}

				// Skip the first one as well.
				top = top->prev;

				if (!top) {
					// Done!
					PLN(L"Done at " << inputPos);
					if (inputPos > matchLast) {
						PLN(L"Picking this one!");
						matchTree = match;
						matchLast = inputPos;
					}
					return;
				}

				PLN(L" => into " << top->iter);

				// Save the match.
				top->match = match;

				// Add new states.
				pqPush(top, top->iter.nextLong(), inputPos, false);
				pqPush(top, top->iter.nextShort(), inputPos, false);
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

			void Parser::pqPush(StackItem *prev, ProductionIter iter, Nat inputPos, Bool first) {
				// TODO: There is some additional logic here to de-duplicate states and handle left recursion.
				if (iter.valid())
					pqPush(create(prev, iter, inputPos, first));
			}

			StackItem *Parser::pqPop() {
				if (pq->filled == 0)
					return null;

				std::pop_heap(pq->v, pq->v + pq->filled, PQCompare());
				StackItem *r = pq->v[--pq->filled];
				pq->v[pq->filled] = null;
				return r;
			}

			StackItem *Parser::create(StackItem *prev, ProductionIter iter, Nat inputPos, Bool first) {
				PLN(L"New state " << inputPos << L", " << iter);
				// TODO: Don't use a hashtable lookup here, we can probably work around it if needed.
				return new (this) StackItem(prodId->get(iter.production()), first, prev, iter, inputPos);
				// return new (this) StackItem(stackId++, first, prev, iter, inputPos);
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
				Node *n = tree(rules->at(matchRule), matchTree, matchFirst, matchLast);
				PVAR(n);
				return n;
			}

			InfoNode *Parser::infoTree() const {
				if (!hasTree())
					return null;

				return infoTree(rules->at(matchRule), matchTree, matchLast);
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

				prodId = new (this) Map<Production *, Nat>();
				Nat maxProdId = 0;
				for (Nat i = 0; i < rules->count(); i++) {
					RuleInfo *info = rules->at(i);
					info->sort();

					for (Nat j = 0; j < info->productions->count(); j++) {
						prodId->put(info->productions->at(j), j);
					}

					maxProdId = max(maxProdId, info->productions->count());
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

				PVAR(token);
				PVAR(part.isRule());
				if (part.isRule()) {
					// Nonterminal.
					RuleToken *rToken = token->asRule();
					dbg_assert(rToken, L"Must be a rule!");
					RuleInfo *info = findRule(rToken->rule);
					dbg_assert(info, L"Must be present!");
					Node *t = tree(info, part.match, part.startPos, last);
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

			Node *Parser::tree(RuleInfo *rule, Tree *root, Nat firstPos, Nat lastPos) const {
				Production *p = rule->productions->at(root->filled);
				Nat repCount = p->repetitionsFor(root->count);
				Node *result = allocNode(p, firstPos, lastPos);

				Nat repStart = firstPos;
				Nat repEnd = lastPos;
				ProductionIter at = p->firstFixed(repCount);
				PVAR(at);
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

			InfoNode *Parser::infoTree(RuleInfo *rule, Tree *root, Nat lastPos) const {
				Production *p = rule->productions->at(root->filled);
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
							RuleToken *token = t->asRule();
							dbg_assert(token, L"Should be a rule!");
							created = infoTree(findRule(token->rule), part.match, next);
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
