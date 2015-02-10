#include "stdafx.h"
#include "SyntaxVariable.h"
#include "SyntaxNode.h"
#include <algorithm>

namespace storm {

	SyntaxVariable::SyntaxVariable(const SrcPos &pos, const String &v) : type(tString), pos(pos), str(new String(v)) {}

	SyntaxVariable::SyntaxVariable(const SrcPos &pos, SyntaxNode *v) : type(tNode), pos(pos), n(v) {}

	SyntaxVariable::~SyntaxVariable() {
		switch (type) {
		case tString:
			delete str;
			break;
		case tNode:
			delete n;
			break;
		default:
			assert(false);
			break;
		}
	}

	String SyntaxVariable::name(Type type) {
		switch (type) {
		case tString:
			return L"string";
		case tNode:
			return L"syntax node";
		default:
			assert(false);
			return L"?";
		}
	}

	void SyntaxVariable::output(wostream &to) const {
		switch (type) {
		case tString:
			if (str)
				to << *str;
			break;
		case tNode:
			if (n)
				to << *n;
			break;
		}
		to << L" @" << pos;
	}

	void SyntaxVariable::orphan() {
		switch (type) {
		case tNode:
			n = null;
			break;
		case tString:
			str = null;
			break;
		}
	}

}
