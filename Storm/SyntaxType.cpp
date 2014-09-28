#include "stdafx.h"
#include "SyntaxType.h"

namespace storm {

	SyntaxType::SyntaxType(const String &name) : name(name) {}

	SyntaxType::~SyntaxType() {
		clear(rules);
	}

	void SyntaxType::output(std::wostream &to) const {
		for (nat i = 0; i < rules.size(); i++) {
			to << name << " : " << *rules[i] << std::endl;
		}
	}

}
