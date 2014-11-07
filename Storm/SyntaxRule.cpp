#include "stdafx.h"
#include "SyntaxRule.h"

namespace storm {

	SyntaxRule::SyntaxRule(const String &name, bool owner) : rName(name), owner(owner), declared() {}

	SyntaxRule::~SyntaxRule() {
		if (owner) {
			clear(options);
		}
	}

	void SyntaxRule::copyDeclaration(const SyntaxRule &o) {
		declared = o.declared;
		params = o.params;
	}

	void SyntaxRule::add(SyntaxOption *rule) {
		options.push_back(rule);
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
