#include "stdafx.h"
#include "Parser.h"

namespace storm {
	namespace syntax {
		namespace ll {

			Parser::Parser() {
				rules = new (this) Array<RuleInfo *>();
				ruleId = new (this) Map<Rule *, Nat>();
				syntaxPrepared = false;
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
				prepare();
				PVAR(str);

				RuleInfo *rule = findRule(root);
				if (!rule)
					return false;

				PLN(L"Parsing...");

				// Try all possible starts, in order.
				for (Nat i = 0; i < rule->productions->count(); i++) {
					Production *p = rule->productions->at(i);
					PLN(L"Trying " << p);
					if (parse(p->firstA(), str, start.offset()))
						return true;
					if (parse(p->firstB(), str, start.offset()))
						return true;
				}

				PLN(L"Failed.");
				return false;
			}

			InfoErrors Parser::parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx) {
				prepare();
				return InfoErrors();
			}

			Bool Parser::parse(ProductionIter iter, Str *str, Nat pos) {
				if (!iter.valid())
					return false;

				StackItem *top = StackItem::branch(engine(), null, iter, pos);

				while (top) {
					Token *token = top->iter.token();
					if (!token) {
						// "reduce" the currently matched portion.
						if (parseReduce(top, str))
							return true;
					} else if (RuleToken *rule = token->asRule()) {
						parseRule(rule, top, str);
					} else if (RegexToken *regex = token->asRegex()) {
						parseRegex(regex, top, str);
					}
				}

				return false;
			}

			Bool Parser::parseReduce(StackItem *&top, Str *str) {
				// Remove the entire branch. Eventually we want to store the parse tree somewhere.
				Nat inputPos = top->inputPos;
				top = top->createdBy;
				if (!top) {
					// We're done!
					return true;
				}

				// Check if 'nextB' is valid. If so, we need to tell the top to try again with that at a later point.
				if (top->iter.nextB().valid()) {
					top->data = inputPos + 1;
				}

				ProductionIter a = top->iter.nextA();
				top = StackItem::follow(engine(), top, top->iter.nextA(), inputPos);
				return false;
			}

			void Parser::parseRule(RuleToken *rule, StackItem *&top, Str *str) {
				// We were completed previously, and we should try to advance using nextB.
				if (top->data > 0) {
					Nat pos = top->data - 1;
					top->data = 0;
					top = StackItem::follow(engine(), top, top->iter.nextB(), pos);
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
						iter = p->firstA();
					} else {
						iter = p->firstB();
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

			void Parser::parseRegex(RegexToken *regex, StackItem *&top, Str *str) {
				if (top->state == 0) {
					// Match the regex
					Nat matched = regex->regex.matchRaw(str, top->inputPos);
					if (matched == Regex::NO_MATCH) {
						// Backtrack.
						top = top->prev;
						return;
					}

					// Store how much we shall advance.
					top->state = 1;
					top->data = matched;

					// Try the next state.
					top = StackItem::follow(engine(), top, top->iter.nextA(), matched);
				} else if (top->state == 1) {
					// We already matched the regex, try to match nextB() instead.
					ProductionIter next = top->iter.nextB();
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

			void Parser::clear() {}

			Bool Parser::hasError() const {
				return false;
			}

			Bool Parser::hasTree() const {
				return false;
			}

			Str::Iter Parser::matchEnd() const {
				return Str::Iter();
			}

			Str *Parser::errorMsg() const {
				return new (this) Str(S("TODO!"));
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

			void Parser::prepare() {
				if (syntaxPrepared)
					return;

				for (Nat i = 0; i < rules->count(); i++) {
					rules->at(i)->sort();
				}

				syntaxPrepared = true;
			}

			MAYBE(RuleInfo *) Parser::findRule(Rule *rule) {
				Nat id = ruleId->get(rule, rules->count());
				if (id < rules->count())
					return rules->at(id);
				else
					return null;
			}

		}
	}
}
