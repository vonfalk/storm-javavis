#include "stdafx.h"
#include "SyntaxNode.h"

namespace storm {

	SyntaxNode::SyntaxNode(const SyntaxOption *rule) : srcRule(rule) {}

	SyntaxNode::~SyntaxNode() {
		clearMap(vars);
	}

	void SyntaxNode::output(wostream &to) const {
		to << *srcRule << " (";
		if (vars.size() > 0) {
			to << endl;
			Indent i(to);
			join(to, vars, L",\n");
			to << endl;
		}
		to << ")";
	}

	void SyntaxNode::add(const String &var, const String &val) {
		try {
			SyntaxVariable *v = find(var, typeOf(var, true));
			v->add(val);
		} catch (SyntaxTypeError e) {
			e.where = srcRule->pos;
			throw e;
		}
	}

	void SyntaxNode::add(const String &var, SyntaxNode *node) {
		try {
			SyntaxVariable *v = find(var, typeOf(var, false));
			v->add(node);
		} catch (SyntaxTypeError e) {
			e.where = srcRule->pos;
			throw e;
		}
	}

	void SyntaxNode::reverseArrays() {
		for (VarMap::iterator i = vars.begin(); i != vars.end(); ++i)
			i->second->reverseArray();
	}

	SyntaxVariable::Type SyntaxNode::typeOf(const String &name, bool isStr) {
		if (name.endsWith(L"[]"))
			return isStr ? SyntaxVariable::tStringArr : SyntaxVariable::tNodeArr;
		else
			return isStr ? SyntaxVariable::tString : SyntaxVariable::tNode;
	}

	SyntaxVariable *SyntaxNode::find(const String &name, SyntaxVariable::Type type) {
		VarMap::iterator i = vars.find(name);
		if (i == vars.end()) {
			SyntaxVariable *v = new SyntaxVariable(type);
			vars.insert(make_pair(name, v));
			return v;
		} else {
			SyntaxVariable *v = i->second;
			if (v->type() != type) {
				SyntaxTypeError e(L"Invalid type of " + name + L": "
								+ SyntaxVariable::name(v->type())
								+ L", expected "
								+ SyntaxVariable::name(type));
				e.where = srcRule->pos;
				throw e;
			}
			return v;
		}
	}

}
