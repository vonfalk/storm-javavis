#include "stdafx.h"
#include "Syntax.h"
#include "Stack.h"
#include "Item.h"

namespace storm {
	namespace syntax {
		namespace glr {

			RuleInfo::RuleInfo() : productions(null), followSet(null), followRules(null), firstRules(null) {}

			void RuleInfo::push(Nat id) {
				if (!productions)
					productions = new (this) Array<Nat>();
				productions->push(id);
			}

			void RuleInfo::follows(Regex regex) {
				if (!followSet)
					followSet = new (this) Set<Regex>();
				followSet->put(regex);
			}

			void RuleInfo::follows(Nat rule) {
				if (!followRules)
					followRules = new (this) Set<Nat>();
				followRules->put(rule);
				if (rule == 0xC000000E)
					DebugBreak();
			}

			void RuleInfo::followsFirst(Nat rule) {
				if (!firstRules)
					firstRules = new (this) Set<Nat>();
				firstRules->put(rule);
			}

			Set<Regex> *RuleInfo::first(Syntax *syntax) {
				Set<Regex> *r = new (syntax) Set<Regex>();
				if (!productions || compFirst)
					return r;

				compFirst = true;

				for (Nat i = 0; i < productions->count(); i++) {
					Item z(syntax, productions->at(i));

					if (z.end()) {
						r->put(follows(syntax));
					} else if (z.isRule(syntax)) {
						RuleInfo *info = syntax->ruleInfo(z.nextRule(syntax));
						if (!info->compFirst)
							r->put(info->first(syntax));
					} else {
						r->put(z.nextRegex(syntax));
					}
				}

				compFirst = false;

				return r;
			}

			Set<Regex> *RuleInfo::follows(Syntax *syntax) {
				if (!followSet)
					followSet = new (this) Set<Regex>();
				if (compFollows)
					return followSet;
				compFollows = true;

				typedef Set<Nat>::Iter Iter;

				if (followRules) {
					for (Iter i = followRules->begin(), end = followRules->end(); i != end; ++i) {
						RuleInfo *info = syntax->ruleInfo(i.v());
						followSet->put(info->follows(syntax));
					}
					followRules = null;
				}

				if (firstRules) {
					for (Iter i = firstRules->begin(), end = firstRules->end(); i != end; ++i) {
						RuleInfo *info = syntax->ruleInfo(i.v());
						followSet->put(info->first(syntax));
					}
					firstRules = null;
				}

				compFollows = false;
				return followSet;
			}


			/**
			 * Syntax.
			 */

			Syntax::Syntax() {
				rLookup = new (this) Map<Rule *, Nat>();
				pLookup = new (this) Map<Production *, Nat>();
				rules = new (this) Array<Rule *>();
				ruleProds = new (this) Array<RuleInfo *>();
				repRuleProds = new (this) Array<RuleInfo *>();
				productions = new (this) Array<Production *>();
			}

			Nat Syntax::add(Rule *rule) {
				Nat id = rLookup->get(rule, rules->count());
				if (id < rules->count())
					return id;

				// New rule!
				rules->push(rule);
				ruleProds->push(new (this) RuleInfo());
				rLookup->put(rule, id);
				return id;
			}

			Nat Syntax::add(Production *p) {
				Nat id = pLookup->get(p, productions->count());
				if (id < productions->count())
					return id;

				// New production!
				productions->push(p);
				repRuleProds->push(null);
				pLookup->put(p, id);

				if (!rLookup->has(p->rule()))
					add(p->rule());

				Nat ruleId = lookup(p->rule());
				ruleProds->at(ruleId)->push(id);

				addFollows(p);

				return id;
			}

			void Syntax::addFollows(Production *p) {
				addFollows(Item(this, p), p);
			}

			void Syntax::addFollows(const Item &start, Production *p) {
				for (Item at = start; !at.end(); at = at.next(p)) {
					if (at.pos == Item::specialPos && specialProd(at.id) == 0)
						addFollows(Item(this, baseProd(at.id) | prodRepeat), p);

					if (!at.isRule(this))
						continue;

					Nat rule = at.nextRule(this);
					if (specialRule(rule))
						// We compute these on the fly when we need them...
						continue;

					addFollows(ruleInfo(rule), at.next(p));
				}
			}

			void Syntax::addFollows(RuleInfo *into, const Item &next) {
				if (next.end()) {
					// The follow set of 'rule' is the follow set of this production.
					into->follows(next.rule(this));
				} else if (next.isRule(this)) {
					// The follow set of 'rule' is the first set of 'next.nextRule'
					into->followsFirst(next.nextRule(this));
				} else {
					// The follow set shall also include this regex.
					into->follows(next.nextRegex(this));
				}
			}

			Nat Syntax::lookup(Rule *r) {
				Nat id = rLookup->get(r, rules->count());
				if (id >= rules->count()) {
					// We need to add it...
					id = add(r);
				}
				return id;
			}

			Nat Syntax::lookup(Production *p) {
				Nat id = pLookup->get(p, productions->count());
				if (id >= productions->count()) {
					id = add(p);
				}
				return id;
			}

			RuleInfo *Syntax::ruleInfo(Nat rule) {
				switch (specialRule(rule)) {
				case ruleRepeat: {
					Nat prod = baseRule(rule);
					RuleInfo *info = repRuleProds->at(prod);
					if (!info) {
						info = new (this) RuleInfo();
						repRuleProds->at(prod) = info;

						info->push(prod | prodEpsilon);
						info->push(prod | prodRepeat);

						// We only need to consider the repeat production for the follow set.
						addFollows(info, Item(this, prod | prodRepeat).next(this));
						addFollows(info, special(prod).next(this));
					}

					return info;
				}
				case ruleESkip: {
					Nat prod = baseRule(rule);
					RuleInfo *info = new (this) RuleInfo();
					info->push(prod | prodESkip);
					// Note: these rules do not need a follow set, as they are treated specially.
					return info;
				}
				default:
					return ruleProds->at(rule);
				}
			}

			Str *Syntax::ruleName(Nat id) const {
				switch (specialRule(id)) {
				case ruleRepeat: {
					Production *p = productions->at(baseRule(id));
					return *p->rule()->identifier() + new (this) Str(L"'");
				}
				case ruleESkip:
					return TO_S(this, L"red" << baseRule(id));
				default:
					return rules->at(id)->identifier();
				}
			}

			Production *Syntax::production(Nat pid) const {
				return productions->at(baseProd(pid));
			}

			Bool Syntax::sameSyntax(Syntax *o) const {
				if (productions->count() != o->productions->count())
					return false;

				for (Nat i = 0; i < productions->count(); i++) {
					if (!o->pLookup->has(productions->at(i)))
						return false;
				}

				return true;
			}

		}
	}
}
