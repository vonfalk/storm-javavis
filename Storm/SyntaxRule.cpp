#include "stdafx.h"
#include "SyntaxRule.h"

#include "SyntaxType.h"

namespace storm {

	SyntaxRule::SyntaxRule() : owner(null), repStart(0), repEnd(0), repType(rNone) {}

	SyntaxRule::~SyntaxRule() {
		::clear(tokens);
	}

	void SyntaxRule::clear() {
		::clear(tokens);
		repStart = repEnd = 0;
		repType = rNone;
		matchFn = L"";
		matchFnParams.clear();
	}

	String SyntaxRule::type() const {
		if (owner)
			return owner->name();
		else
			return L"";
	}

	void SyntaxRule::outputRep(std::wostream &to, nat i, nat marker) const {
		if (repStart == i && repType != rNone)
			to << L"(";
		if (marker == i)
			to << L"<>";
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
		output(to, -1);
	}

	void SyntaxRule::output(wostream &to, nat marker) const {
		if (owner)
			to << owner->name() << L" : ";

		for (nat i = 0; i < tokens.size(); i++) {
			if (i > 0)
				to << L" ";

			outputRep(to, i, marker);
			to << *tokens[i];

			// Not too nice, but gives good outputs!
			if (i + 1 < tokens.size()) {
				if (dynamic_cast<DelimToken*>(tokens[i+1])) {
					i++;
					outputRep(to, i, marker);
					to << L",";
				} else {
					to << L" -";
				}
			}
		}

		outputRep(to, tokens.size(), marker);

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

	/**
	 * Iterator.
	 */
	RuleIter::RuleIter() : ruleP(null), tokenId(0) {}

	RuleIter::RuleIter(SyntaxRule &rule) : ruleP(&rule), tokenId(0), repCount(0) {}

	RuleIter::RuleIter(SyntaxRule *rule, nat token, nat rep) : ruleP(rule), tokenId(token), repCount(rep) {}

	bool RuleIter::valid() const {
		return ruleP != null;
	}

	bool RuleIter::end() const {
		return (ruleP == null)
			|| (tokenId >= ruleP->tokens.size());
	}

	RuleIter RuleIter::nextA() const {
		bool start = (tokenId + 1) == ruleP->repStart;
		nat rep = start ? repCount + 1 : repCount;
		return RuleIter(ruleP, tokenId + 1, rep);
	}

	RuleIter RuleIter::nextB() const {
		bool start = (tokenId + 1) == ruleP->repStart;
		bool end = (tokenId + 1) == ruleP->repEnd;

		switch (ruleP->repType) {
		case SyntaxRule::rNone:
			return RuleIter();
		case SyntaxRule::rZeroOne:
			if (start)
				return RuleIter(ruleP, ruleP->repEnd, 0);
			return RuleIter();
		case SyntaxRule::rZeroPlus:
			if (start)
				return RuleIter(ruleP, ruleP->repEnd, 0);
			if (end)
				return RuleIter(ruleP, ruleP->repStart, repCount + 1);
			return RuleIter();
		case SyntaxRule::rOnePlus:
			if (end)
				return RuleIter(ruleP, ruleP->repStart, repCount + 1);
			return RuleIter();
		default:
			assert(false);
			return RuleIter();
		}
	}

	SyntaxToken *RuleIter::token() const {
		if (end())
			return null;
		return ruleP->tokens[tokenId];
	}

	void RuleIter::output(wostream &to) const {
		to << "{";
		ruleP->output(to, tokenId);
		to << ", " << repCount << "}";
	}
}
