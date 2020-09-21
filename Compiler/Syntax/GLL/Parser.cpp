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

				// Try all possible starts, in order. Try until we have consumed the entire string, or tried all of them.
				for (Nat i = 0; i < rule->productions->count(); i++) {
					Production *p = rule->productions->at(i);
					matchProd = i;
					PLN(L"Trying " << p);
					if (parse(p->firstLong(), start.offset()))
						if (matchLast >= str->peekLength())
							return true;
					if (parse(p->firstShort(), start.offset()))
						if (matchLast >= str->peekLength())
							return true;
				}

				// We found something!
				if (matchTree)
					return true;

				PLN(L"Failed.");
				return false;
			}

			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx) {
				prepare(str, file);
				return InfoErrors();
			}

			Bool Parser::parse(ProductionIter iter, Nat pos) {
				if (!iter.valid())
					return false;

				StackItem *top = StackItem::branch(engine(), null, iter, pos);

				while (top) {
					Token *token = top->iter.token();
					if (!token) {
						// "reduce" the currently matched portion.
						if (parseReduce(top))
							return true;
					} else if (RuleToken *rule = token->asRule()) {
						parseRule(rule, top);
					} else if (RegexToken *regex = token->asRegex()) {
						parseRegex(regex, top);
					}
				}

				return false;
			}

			Bool Parser::parseReduce(StackItem *&top) {
				// Find the length of this "branch".
				StackItem *newTop = top->createdBy;
				Nat count = 0;
				for (StackItem *at = top->prev; at != newTop; at = at->prev)
					count++;

				// Create a Tree-array for this branch.
				GcArray<TreePart> *match = runtime::allocArray<TreePart>(engine(), treeArrayType, count);
				match->filled = prodId->get(top->iter.production());

				for (StackItem *at = top->prev; at != newTop; at = at->prev) {
					count--;

					if (at->match) {
						match->v[count] = TreePart(at->match, at->inputPos);
					} else {
						match->v[count] = TreePart(at->inputPos);
					}
				}

				// Remove the entire branch. Eventually we want to store the parse tree somewhere.
				Nat inputPos = top->inputPos;
				top = newTop;
				if (!top) {
					// We're done!
					PLN(L"Done at " << inputPos);
					if (inputPos > matchLast) {
						matchTree = match;
						matchLast = inputPos;
					}
					return true;
				}

				// Save the match.
				top->match = match;

				// Check if 'nextShort' is valid. If so, we need to tell the top to try again with that at a later point.
				if (top->iter.nextShort().valid()) {
					top->data = inputPos + 1;
				}

				ProductionIter a = top->iter.nextLong();
				top = StackItem::follow(engine(), top, top->iter.nextLong(), inputPos);
				return false;
			}

			void Parser::parseRule(RuleToken *rule, StackItem *&top) {
				// We were completed previously, and we should try to advance using nextShort.
				if (top->data > 0) {
					Nat pos = top->data - 1;
					top->data = 0;
					top = StackItem::follow(engine(), top, top->iter.nextShort(), pos);
					return;
				}

				RuleInfo *info = findRule(rule->rule);
				if (!info) {
					top = top->prev;
					return;
				}

				if (top->state >= info->productions->count() * 2) {
					return;
				}

				Nat prods = info->productions->count();
				while (top->state < prods * 2) {
					Production *p = info->productions->at(top->state / 2);
					ProductionIter iter;
					if ((top->state & 0x1) == 0x0) {
						iter = p->firstLong();
					} else {
						iter = p->firstShort();
					}

					// Remember to advance next time we get here.
					top->state++;

					// If it is valid, go there.
					if (iter.valid()) {
						top = StackItem::branch(engine(), top, iter, top->inputPos);
						return;
					}
				}

				// At the end, backtrack.
				top = top->prev;
			}

			void Parser::parseRegex(RegexToken *regex, StackItem *&top) {
				if (top->state == 0) {
					// Match the regex
					Nat matched = regex->regex.matchRaw(src, top->inputPos);
					if (matched == Regex::NO_MATCH) {
						// Backtrack.
						top = top->prev;
						return;
					}

					// Store how much we shall advance.
					top->state = 1;
					top->data = matched;

					// Try the next state.
					top = StackItem::follow(engine(), top, top->iter.nextLong(), matched);
				} else if (top->state == 1) {
					// We already matched the regex, try to match nextShort() instead.
					ProductionIter next = top->iter.nextShort();
					if (!next.valid()) {
						// Backtrack.
						top = top->prev;
						return;
					}

					Nat matched = top->data;
					top->state = 2;

					// Try the next state.
					top = StackItem::follow(engine(), top, next, matched);
				} else {
					// If we get here, we don't have anything else to do. Just backtrack.
					top = top->prev;
				}
			}

			void Parser::clear() {
				url = null;
				src = null;
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
				Node *n = tree(rules->at(matchRule), matchProd, matchTree, matchFirst, matchLast);
				PVAR(n);
				return n;
			}

			InfoNode *Parser::infoTree() const {
				if (!hasTree())
					return null;

				return infoTree(rules->at(matchRule), matchProd, matchTree, matchLast);
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
				matchTree = null;
				matchLast = 0;

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

				if (part.isRule()) {
					// Nonterminal.
					RuleToken *rToken = token->asRule();
					dbg_assert(rToken, L"Must be a rule!");
					RuleInfo *info = findRule(rToken->rule);
					dbg_assert(info, L"Must be present!");
					Node *t = tree(info, part.productionId(), part.match, part.startPos, last);
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

			Node *Parser::tree(RuleInfo *rule, Nat prodId, Tree *root, Nat firstPos, Nat lastPos) const {
				Production *p = rule->productions->at(prodId);
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

			InfoNode *Parser::infoTree(RuleInfo *rule, Nat prodId, Tree *root, Nat lastPos) const {
				Production *p = rule->productions->at(prodId);
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
							created = infoTree(findRule(token->rule), part.productionId(), part.match, next);
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
