#include "stdafx.h"
#include "SyntaxOption.h"

#include "SyntaxRule.h"

namespace storm {

	SyntaxOption::SyntaxOption(const SrcPos &pos, Scope *scope)
		: scope(scope), pos(pos), owner(null), repStart(0), repEnd(0), repType(rNone) {}

	SyntaxOption::~SyntaxOption() {
		::clear(tokens);
	}

	void SyntaxOption::clear() {
		::clear(tokens);
		repStart = repEnd = 0;
		repType = rNone;
		matchFn = Name();
		matchFnParams.clear();
	}

	String SyntaxOption::rule() const {
		if (owner)
			return owner->name();
		else
			return L"";
	}

	int SyntaxOption::outputRep(std::wostream &to, nat i, nat marker) const {
		int any = 0;
		if (repStart == i && repType != rNone) {
			any |= 1;
			to << L" (";
		}
		if (marker == i)
			to << L"<>";
		if (repEnd == i) {
			switch (repType) {
			case rZeroOne:
				any |= 2;
				to << L")?";
				break;
			case rZeroPlus:
				any |= 2;
				to << L")*";
				break;
			case rOnePlus:
				any |= 2;
				to << L")+";
				break;
			}
		}
		return any;
	}

	void SyntaxOption::output(std::wostream &to) const {
		output(to, -1);
	}

	void SyntaxOption::output(wostream &to, nat marker) const {
		if (owner)
			to << owner->name();

		if (matchFn.any()) {
			to << L" => ";
			to << matchFn << L"(";
			for (nat i = 0; i < matchFnParams.size(); i++) {
				if (i > 0) to << L", ";
				to << matchFnParams[i];
			}
			to << L") : ";
		}

		bool lastComma = false;
		bool lastRep = true;
		for (nat i = 0; i < tokens.size(); i++) {
			switch (outputRep(to, i, marker)) {
			case 0:
				break;
			case 1:
				lastComma = false;
				lastRep = true;
				break;
			case 2:
			case 3:
				lastComma = false;
				lastRep = false;
				break;
			}

			if (!lastComma && as<DelimToken>(tokens[i])) {
				to << L",";
				lastComma = true;
			} else {
				if (i > 0) {
					if (!lastRep)
						to << L" ";
					if (!lastComma && !lastRep)
						to << L"- ";
				}
				to << *tokens[i];
				lastComma = false;
			}

			lastRep = false;
		}

		outputRep(to, tokens.size(), marker);

		to << L";";
	}

	void SyntaxOption::add(SyntaxToken *token) {
		tokens.push_back(token);
	}

	void SyntaxOption::startRepeat() {
		repStart = tokens.size();
	}

	void SyntaxOption::endRepeat(Repeat r) {
		repEnd = tokens.size();
		repType = r;
	}


	/**
	 * Iterator.
	 */
	RuleIter::RuleIter() : ruleP(null), tokenId(0) {}

	RuleIter::RuleIter(SyntaxOption &rule) : ruleP(&rule), tokenId(0), repCount(0) {}

	RuleIter::RuleIter(SyntaxOption *rule, nat token, nat rep) : ruleP(rule), tokenId(token), repCount(rep) {}

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
		case SyntaxOption::rNone:
			return RuleIter();
		case SyntaxOption::rZeroOne:
			if (start)
				return RuleIter(ruleP, ruleP->repEnd, 0);
			return RuleIter();
		case SyntaxOption::rZeroPlus:
			if (start)
				return RuleIter(ruleP, ruleP->repEnd, 0);
			if (end)
				return RuleIter(ruleP, ruleP->repStart, repCount + 1);
			return RuleIter();
		case SyntaxOption::rOnePlus:
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
