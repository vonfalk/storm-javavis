#include "stdafx.h"
#include "Syntax.h"
#include "Stack.h"

namespace storm {
	namespace syntax {
		namespace glr {

			RuleInfo::RuleInfo() : productions(null) {}

			void RuleInfo::push(Engine &e, Nat id) {
				if (!productions)
					productions = new (e) Array<Nat>();
				productions->push(id);
			}

			void push(EnginePtr e, RuleInfo &to, Nat id) {
				to.push(e.v, id);
			}

			/**
			 * Syntax.
			 */

			Syntax::Syntax() {
				rLookup = new (this) Map<Rule *, Nat>();
				pLookup = new (this) Map<Production *, Nat>();
				rules = new (this) Array<Rule *>();
				ruleProds = new (this) Array<RuleInfo>();
				productions = new (this) Array<Production *>();
			}

			void Syntax::add(Rule *rule) {
				if (rLookup->has(rule))
					return;

				// New rule!
				Nat id = rules->count();
				rules->push(rule);
				ruleProds->push(RuleInfo());
				rLookup->put(rule, id);
			}

			void Syntax::add(Production *p) {
				if (pLookup->has(p))
					return;

				// New production!
				Nat id = productions->count();
				productions->push(p);
				pLookup->put(p, id);

				if (!rLookup->has(p->rule()))
					add(p->rule());

				Nat ruleId = lookup(p->rule());
				ruleProds->at(ruleId).push(engine(), id);
			}

			Nat Syntax::lookup(Rule *r) const {
				return rLookup->get(r);
			}

			Nat Syntax::lookup(Production *p) const {
				return pLookup->get(p);
			}

			RuleInfo Syntax::ruleInfo(Nat rule) const {
				if (specialRule(rule)) {
					Nat prod = baseRule(rule);
					RuleInfo info;
					info.push(engine(), prod | prodEpsilon);
					info.push(engine(), prod | prodRepeat);
					return info;
				} else {
					return ruleProds->at(rule);
				}
			}

			Str *Syntax::ruleName(Nat id) const {
				if (specialRule(id)) {
					Production *p = productions->at(baseRule(id));
					return *p->rule()->identifier() + new (this) Str(L"'");
				} else {
					Rule *rule = rules->at(id);
					return rule->identifier();
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
