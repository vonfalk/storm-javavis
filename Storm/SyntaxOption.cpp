#include "stdafx.h"
#include "SyntaxOption.h"

#include "SyntaxRule.h"

namespace storm {

	SyntaxOption::SyntaxOption(const SrcPos &pos, const Scope &scope, const String &owner)
		: scope(scope), pos(pos), owner(owner), markStart(0), markEnd(0), markType(mNone) {}

	SyntaxOption::~SyntaxOption() {
		_ASSERT(_CrtCheckMemory());
		::clear(tokens);
		_ASSERT(_CrtCheckMemory());
	}

	void SyntaxOption::clear() {
		::clear(tokens);
		markStart = markEnd = 0;
		markType = mNone;
		matchFn = Name();
		matchFnParams.clear();
		markCapture = L"";
	}

	int SyntaxOption::outputMark(std::wostream &to, nat i, nat marker) const {
		int any = 0;
		if (markStart == i && markType != mNone) {
			any |= 1;
			to << L" (";
		}
		if (marker == i)
			to << L"<>";
		if (markEnd == i) {
			switch (markType) {
			case mRepZeroOne:
				any |= 2;
				to << L")?";
				break;
			case mRepZeroPlus:
				any |= 2;
				to << L")*";
				break;
			case mRepOnePlus:
				any |= 2;
				to << L")+";
				break;
			case mCapture:
				any |= 2;
				to << L") " << markCapture;
				break;
			}
		}
		return any;
	}

	void SyntaxOption::output(std::wostream &to) const {
		output(to, -1);
	}

	void SyntaxOption::output(wostream &to, nat marker) const {
		to << owner;

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
		bool lastMark = true;
		for (nat i = 0; i < tokens.size(); i++) {
			switch (outputMark(to, i, marker)) {
			case 0:
				break;
			case 1:
				lastComma = false;
				lastMark = true;
				break;
			case 2:
			case 3:
				lastComma = false;
				lastMark = false;
				break;
			}

			if (!lastComma && as<DelimToken>(tokens[i])) {
				to << L",";
				lastComma = true;
			} else {
				if (i > 0) {
					if (!lastMark)
						to << L" ";
					if (!lastComma && !lastMark)
						to << L"- ";
				}
				to << *tokens[i];
				lastComma = false;
			}

			lastMark = false;
		}

		outputMark(to, tokens.size(), marker);

		to << L";";
	}

	void SyntaxOption::add(SyntaxToken *token) {
		tokens.push_back(token);
	}

	void SyntaxOption::startMark() {
		markStart = tokens.size();
	}

	void SyntaxOption::endMark(Mark r) {
		markEnd = tokens.size();
		markType = r;
	}

	void SyntaxOption::endMark(const String &to) {
		endMark(mCapture);
		markCapture = to;
	}


	/**
	 * Iterator.
	 */
	OptionIter::OptionIter() : optionP(null), tokenId(0) {}

	OptionIter::OptionIter(SyntaxOption *option, nat token, nat rep) : optionP(option), tokenId(token), repCount(rep) {}

	OptionIter OptionIter::firstA(SyntaxOption &option) {
		return OptionIter(&option, 0, 0);
	}

	OptionIter OptionIter::firstB(SyntaxOption &option) {
		if (option.markStart > 0)
			return OptionIter();

		switch (option.markType) {
		case SyntaxOption::mNone:
		case SyntaxOption::mCapture:
			return OptionIter();
		case SyntaxOption::mRepZeroOne:
		case SyntaxOption::mRepZeroPlus:
			return OptionIter(&option, option.markEnd, 0);
		case SyntaxOption::mRepOnePlus:
			return OptionIter();
		default:
			assert(false);
			return OptionIter();
		}
	}

	bool OptionIter::valid() const {
		return optionP != null;
	}

	bool OptionIter::end() const {
		return (optionP == null)
			|| (tokenId >= optionP->tokens.size());
	}

	bool OptionIter::captureBegin() const {
		return valid()
			&& (optionP->markType == SyntaxOption::mCapture)
			&& (optionP->markStart == tokenId);
	}

	bool OptionIter::captureEnd() const {
		return valid()
			&& (optionP->markType == SyntaxOption::mCapture)
			&& (optionP->markEnd == tokenId);
	}

	OptionIter OptionIter::nextA() const {
		bool start = (tokenId + 1) == optionP->markStart;
		nat rep = start ? repCount + 1 : repCount;
		return OptionIter(optionP, tokenId + 1, rep);
	}

	OptionIter OptionIter::nextB() const {
		bool start = (tokenId + 1) == optionP->markStart;
		bool end = (tokenId + 1) == optionP->markEnd;

		switch (optionP->markType) {
		case SyntaxOption::mNone:
		case SyntaxOption::mCapture:
			return OptionIter();
		case SyntaxOption::mRepZeroOne:
			if (start)
				return OptionIter(optionP, optionP->markEnd, 0);
			return OptionIter();
		case SyntaxOption::mRepZeroPlus:
			if (start)
				return OptionIter(optionP, optionP->markEnd, 0);
			if (end)
				return OptionIter(optionP, optionP->markStart, repCount + 1);
			return OptionIter();
		case SyntaxOption::mRepOnePlus:
			if (end)
				return OptionIter(optionP, optionP->markStart, repCount + 1);
			return OptionIter();
		default:
			assert(false);
			return OptionIter();
		}
	}

	SyntaxToken *OptionIter::token() const {
		if (end())
			return null;
		return optionP->tokens[tokenId];
	}

	void OptionIter::output(wostream &to) const {
		to << "{";
		optionP->output(to, tokenId);
		to << ", " << repCount << "}";
	}
}
