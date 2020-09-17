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

				StackItem *top = new (this) StackItem(null, iter, pos);

				while (top) {
					Token *token = top->iter.token();
					if (!token) {
						// We're at the end. Do something intelligent.
						TODO(L"FIXME");
						return false;
					} else if (RuleToken *rule = token->asRule()) {
						parseRule(rule, top, str);
					} else if (RegexToken *regex = token->asRegex()) {
						parseRegex(regex, top, str);
					}
				}

				return true;
			}

			void Parser::parseRule(RuleToken *rule, StackItem *&top, Str *str) {
				// To make it terminate.
				top = null;
			}

			void Parser::parseRegex(RegexToken *regex, StackItem *&top, Str *str) {
				PLN(L"Matching " << regex << "...");
				if (top->state == 0) {
					// Match the regex
					Nat r = regex->regex.matchRaw(str, top->inputPos);
					if (r == Regex::NO_MATCH) {
						// Backtrack.
						top = top->prev;
						return;
					}

					// Store how much we shall advance.
					top->data = r;

					// Try the next state.
					top = new (this) StackItem(top, top->iter.nextA(), top->inputPos + r);
				} else if (top->state == 1) {
					// We already matched the regex, try to match nextB() instead.
					ProductionIter next = top->iter.nextB();
					if (!next.valid()) {
						// Backtrack.
						top = top->prev;
					}

					top->state = 2;

					// Try the next state.
					top = new (this) StackItem(top, next, top->inputPos + top->data);
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
				return null;
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
