#include "stdafx.h"
#include "SyntaxRule.h"

namespace storm {

	SyntaxRule::SyntaxRule() : repStart(0), repEnd(0), repType(rNone) {}

	SyntaxRule::~SyntaxRule() {
		clear(tokens);
	}

	void SyntaxRule::outputRep(std::wostream &to, nat i) const {
		if (repStart == i && repType != rNone)
			to << L"(";
		if (repEnd == i) {
			switch (repType) {
			case rZeroOne:
				to << L")?";
				break;
			case rZeroPlus:
				to << L")*";
				break;
			case rOnePlus:
				to << L")+";
				break;
			}
		}
	}

	void SyntaxRule::output(std::wostream &to) const {
		for (nat i = 0; i < tokens.size(); i++) {
			if (i > 0)
				to << L" ";

			outputRep(to, i);
			to << *tokens[i];

			// Not too nice, but gives good outputs!
			if (i + 1 < tokens.size()) {
				if (dynamic_cast<WhitespaceToken*>(tokens[i+1])) {
					i++;
					outputRep(to, i);
					to << L",";
				} else {
					to << L" -";
				}
			}
		}

		outputRep(to, tokens.size());

		if (matchFn.any()) {
			to << L" = " << matchFn << L"(";
			for (nat i = 0; i < matchFnParams.size(); i++) {
				if (i > 0) to << L", ";
				to << matchFnParams[i];
			}
			to << L")";
		}
	}

	void SyntaxRule::add(SyntaxToken *token) {
		tokens.push_back(token);
	}

	void SyntaxRule::startRepeat() {
		repStart = tokens.size();
	}

	void SyntaxRule::endRepeat(Repeat r) {
		repEnd = tokens.size();
		repType = r;
	}

	void SyntaxRule::setMatchFn(const String &name, const vector<String> &params) {
		matchFn = name;
		matchFnParams = params;
	}

}
