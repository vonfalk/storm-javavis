#include "stdafx.h"
#include "SyntaxType.h"

namespace storm {

	SyntaxType::SyntaxType(const String &name) : tName(name), outputStr(null) {}

	SyntaxType::~SyntaxType() {
		clear(rules);
		delete outputStr;
	}

	void SyntaxType::setOutput(const String &str) {
		if (outputStr == null)
			outputStr = new String(str);
		else
			*outputStr = str;
	}

	void SyntaxType::add(SyntaxRule *rule) {
		rules.push_back(rule);
		rule->owner = this;
	}

	void SyntaxType::output(std::wostream &to) const {
		for (nat i = 0; i < rules.size(); i++) {
			to << *rules[i] << endl;
		}
		if (outputStr)
			to << tName << " => \"" << *outputStr << "\"" << endl;
	}

}
