#include "stdafx.h"
#include "SyntaxRule.h"

namespace storm {

	SyntaxRule::SyntaxRule(const String &name) : rName(name) {}

	SyntaxRule::~SyntaxRule() {
		clear(options);
	}

	void SyntaxRule::add(SyntaxOption *rule) {
		options.push_back(rule);
		rule->owner = this;
	}

	void SyntaxRule::output(std::wostream &to) const {
		to << rName << L"(";
		join(to, params, L", ");
		to << L")" << endl;

		for (nat i = 0; i < options.size(); i++) {
			to << *options[i] << endl;
		}
	}

	wostream &operator <<(wostream &to, const SyntaxRule::Param &p) {
		return to << p.type << L" " << p.name;
	}

}
