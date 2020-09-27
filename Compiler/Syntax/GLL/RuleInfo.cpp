#include "stdafx.h"
#include "RuleInfo.h"

namespace storm {
	namespace syntax {
		namespace gll {

			RuleInfo::RuleInfo(Rule *rule) : rule(rule) {
				productions = new (this) Array<Production *>();
			}

			void RuleInfo::add(Production *p) {
				// The parser checks for uniqueness.
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
