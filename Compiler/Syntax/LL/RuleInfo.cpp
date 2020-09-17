#include "stdafx.h"
#include "RuleInfo.h"

namespace storm {
	namespace syntax {
		namespace ll {

			RuleInfo::RuleInfo(Rule *rule) : rule(rule) {
				productions = new (this) Array<Production *>();
			}

			void RuleInfo::add(ProductionType *production) {
				// A linear search is probably good enough for now. We can easily optimize it if it
				// turns out to be a bottleneck.
				Production *p = production->production;
				for (Nat i = 0; i < productions->count(); i++)
					if (productions->at(i) == p)
						return;

				productions->push(p);
			}

			static Bool sortProductions(Production *a, Production *b) {
				// Largest one first.
				return a->priority > b->priority;
			}

			void RuleInfo::sort() {
				productions->sort(fnPtr(engine(), &sortProductions));
			}

			Bool RuleInfo::sameSyntax(RuleInfo *other) {
				if (productions->count() != other->productions->count())
					return false;

				for (Nat i = 0; i < productions->count(); i++) {
					Production *p = productions->at(i);
					Bool found = false;
					for (Nat j = 0; j < other->productions->count(); j++) {
						if (other->productions->at(j) == p) {
							found = true;
							break;
						}
					}

					if (!found)
						return false;
				}

				return true;
			}

		}
	}
}
