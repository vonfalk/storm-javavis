#include "stdafx.h"
#include "SyntaxRule.h"

namespace storm {

	SyntaxRule::SyntaxRule(const String &name) : tName(name), outputStr(null) {}

	SyntaxRule::~SyntaxRule() {
		clear(options);
		delete outputStr;
	}

	void SyntaxRule::setOutput(const String &str) {
		if (outputStr == null)
			outputStr = new String(str);
		else
			*outputStr = str;
	}

	void SyntaxRule::add(SyntaxOption *rule) {
		options.push_back(rule);
		rule->owner = this;
	}

	void SyntaxRule::output(std::wostream &to) const {
		for (nat i = 0; i < options.size(); i++) {
			to << *options[i] << endl;
		}
		if (outputStr)
			to << tName << " => \"" << *outputStr << "\"" << endl;
	}

}
