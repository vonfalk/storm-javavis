#include "stdafx.h"
#include "SyntaxSet.h"
#include "Package.h"
#include "Exception.h"

namespace storm {

	SyntaxSet::SyntaxSet() {}

	SyntaxSet::~SyntaxSet() {
		clearMap(rules);
	}

	void SyntaxSet::add(Par<Package> p) {
		// TODO: Keep a live reference to the package!
		Package *pkg = p.borrow();
		if (added.count(pkg))
			return;
		added.insert(pkg);

		const SyntaxRules &rules = pkg->syntax();
		for (SyntaxRules::iterator i = rules.begin(); i != rules.end(); ++i) {
			add(i->first, i->second);
		}
	}

	void SyntaxSet::add(const String &path, SyntaxRule *rule) {
		SyntaxRule *dest = safeRule(path);

		if (!rule->declared.unknown()) {
			if (!dest->declared.unknown())
				throw SyntaxError(rule->declared,
								L"The rule " + ::toS(path) +
								L" was previously declared at " +
								::toS(dest->declared));
			dest->copyDeclaration(*rule);
		}

		for (nat i = 0; i < rule->size(); i++) {
			dest->add((*rule)[i]);
		}
	}

	SyntaxRule *SyntaxSet::safeRule(const String &name) {
		SyntaxRule *r = rule(name);
		if (r == null) {
			r = new SyntaxRule(name, Scope(), false);
			rules.insert(make_pair(name, r));
		}
		return r;
	}

	SyntaxRule *SyntaxSet::rule(const String &name) {
		RMap::iterator i = rules.find(name);
		if (i == rules.end())
			return null;
		return i->second;
	}

	void SyntaxSet::output(wostream &to) const {
		for (RMap::const_iterator i = rules.begin(); i != rules.end(); i++) {
			to << *i->second << endl;
		}
	}

}
