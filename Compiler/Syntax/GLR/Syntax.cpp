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
				rules = new (this) Map<Rule *, RuleInfo>();
				lookup = new (this) Map<Production *, Nat>();
				productions = new (this) Array<Production *>();
			}

			void Syntax::add(Rule *rule) {
				// This creates the rule data if it is not already there.
				rules->at(rule);
			}

			void Syntax::add(Production *p) {
				if (lookup->has(p))
					return;

				// New production!
				Nat id = productions->count();
				productions->push(p);
				lookup->put(p, id);
				rules->at(p->rule()).push(engine(), id);
			}

			Bool Syntax::sameSyntax(Syntax *o) {
				if (productions->count() != o->productions->count())
					return false;

				for (Nat i = 0; i < productions->count(); i++) {
					if (!o->lookup->has(productions->at(i)))
						return false;
				}

				return true;
			}

		}
	}
}
