#include "stdafx.h"
#include "Syntax.h"

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
				rules = new (this) Array<RuleInfo>();
				productions = new (this) Array<Production *>();
			}

			void Syntax::add(Rule *rule) {
				if (rLookup->has(rule))
					return;

				// New rule!
				Nat id = rules->count();
				rules->push(RuleInfo());
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
				rules->at(ruleId).push(engine(), id);
			}

			Nat Syntax::lookup(Rule *r) {
				return rLookup->get(r);
			}

			Nat Syntax::lookup(Production *p) {
				return pLookup->get(p);
			}

			RuleInfo Syntax::ruleInfo(Nat rule) {
				if (specialRule(rule)) {
					Nat prod = baseRule(rule);
					RuleInfo info;
					info.push(engine(), prod | prodEpsilon);
					info.push(engine(), prod | prodRepeat);
					return info;
				} else {
					return rules->at(rule);
				}
			}

			Production *Syntax::production(Nat pid) {
				return productions->at(baseProd(pid));
			}

			Bool Syntax::sameSyntax(Syntax *o) {
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
