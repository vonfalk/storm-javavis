#include "stdafx.h"
#include "SyntaxVariable.h"
#include "SyntaxNode.h"
#include <algorithm>

namespace storm {

	SyntaxVariable::SyntaxVariable(Type t) : vType(t) {
		switch (t) {
		case tString:
			str = null;
			break;
		case tStringArr:
			strArr = new vector<String>();
			break;
		case tNode:
			n = null;
			break;
		case tNodeArr:
			nArr = new vector<SyntaxNode*>();
			break;
		default:
			assert(false);
		}
	}

	SyntaxVariable::~SyntaxVariable() {
		switch (vType) {
		case tString:
			delete str;
			break;
		case tStringArr:
			delete strArr;
			break;
		case tNode:
			delete n;
			break;
		case tNodeArr:
			clear(*nArr);
			delete nArr;
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
		case tStringArr:
			return L"string array";
		case tNode:
			return L"syntax node";
		case tNodeArr:
			return L"syntax node array";
		default:
			assert(false);
			return L"?";
		}
	}

	void SyntaxVariable::add(const String &v) {
		switch (vType) {
		case tString:
			if (str)
				throw SyntaxTypeError(L"Expected string array, found string.");
			else
				str = new String(v);
			break;
		case tStringArr:
			strArr->push_back(v);
			break;
		default:
			throw SyntaxTypeError(L"Expected string or string array, found " + name(vType));
		}
	}

	void SyntaxVariable::add(SyntaxNode *v) {
		switch (vType) {
		case tNode:
			if (n)
				throw SyntaxTypeError(L"Expected syntax node array, found syntax node.");
			else
				n = v;
			break;
		case tNodeArr:
			nArr->push_back(v);
			break;
		default:
			throw SyntaxTypeError(L"Expected syntax node or syntax node array, found " + name(vType));
		}
	}

	void SyntaxVariable::output(wostream &to) const {
		switch (vType) {
		case tString:
			if (str)
				to << *str;
			break;
		case tStringArr:
			to << "[";
			join(to, *strArr, L", ");
			to << "]";
			break;
		case tNode:
			if (n)
				to << *n;
			break;
		case tNodeArr:
			to << "[";
			if (nArr->size() > 0) {
				to << endl;
				Indent i(to);
				join(to, *nArr, L",\n");
				to << endl;
			}
			to << "]";
			break;
		}
	}

	void SyntaxVariable::orphan() {
		switch (vType) {
		case tNode:
			n = null;
			break;
		case tNodeArr:
			nArr->clear();
			break;
		}
	}

	void SyntaxVariable::reverseArray() {
		switch (vType) {
		case tStringArr:
			std::reverse(strArr->begin(), strArr->end());
			break;
		case tNodeArr:
			std::reverse(nArr->begin(), nArr->end());
			break;
		}
	}
}
