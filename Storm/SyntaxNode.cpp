#include "stdafx.h"
#include "SyntaxNode.h"

namespace storm {

	SyntaxNode::SyntaxNode(const SyntaxOption *option) : option(option), invocations(mInvocations) {}

	SyntaxNode::~SyntaxNode() {
		for (VarMap::iterator i = vars.begin(); i != vars.end(); ++i)
			delete i->second.value;
		for (nat i = 0; i < mInvocations.size(); i++)
			delete mInvocations[i].val.value;
	}

	void SyntaxNode::output(wostream &to) const {
		to << *option << " (";
		if (vars.size() > 0) {
			to << endl;
			Indent i(to);
			join(to, vars, L",\n");
			to << endl;
		}
		if (invocations.size() > 0) {
			to << endl;
			Indent i(to);
			join(to, invocations, L",\n");
			to << endl;
		}
		to << ")";
	}

	wostream &operator <<(wostream &to, const SyntaxNode::Var &f) {
		to << L"(";
		join(to, f.params, L", ");
		to << L") -> " << *f.value;
		return to;
	}

	wostream &operator <<(wostream &to, const SyntaxNode::Invocation &f) {
		return to << f.member << L" -> " << f.val;
	}

	void SyntaxNode::add(const String &var, const String &val, const vector<String> &params) {
		try {
			Var &v = find(var, typeOf(var, true));
			v.params = params;
			v.value->add(val);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::add(const String &var, SyntaxNode *node, const vector<String> &params) {
		try {
			Var &v = find(var, typeOf(var, false));
			v.params = params;
			v.value->add(node);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::invoke(const String &m, const String &val, const vector<String> &params) {
		try {
			SyntaxVariable *sv = new SyntaxVariable(SyntaxVariable::tString);
			Invocation i = { m, sv, params };
			mInvocations.push_back(i);
			sv->add(val);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::invoke(const String &m, SyntaxNode *val, const vector<String> &params) {
		try {
			SyntaxVariable *sv = new SyntaxVariable(SyntaxVariable::tNode);
			Invocation i = { m, sv, params };
			mInvocations.push_back(i);
			sv->add(val);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::reverseArrays() {
		for (VarMap::iterator i = vars.begin(); i != vars.end(); ++i)
			i->second.value->reverseArray();
	}

	SyntaxVariable::Type SyntaxNode::typeOf(const String &name, bool isStr) {
		if (name.endsWith(L"[]"))
			return isStr ? SyntaxVariable::tStringArr : SyntaxVariable::tNodeArr;
		else
			return isStr ? SyntaxVariable::tString : SyntaxVariable::tNode;
	}

	SyntaxNode::Var &SyntaxNode::find(const String &name, SyntaxVariable::Type type) {
		VarMap::iterator i = vars.find(name);
		if (i == vars.end()) {
			SyntaxVariable *v = new SyntaxVariable(type);
			Var val = { v };
			return vars.insert(make_pair(name, val)).first->second;
		} else {
			Var &v = i->second;
			if (v.value->type != type) {
				SyntaxTypeError e(L"Invalid type of " + name + L": "
								+ SyntaxVariable::name(v.value->type)
								+ L", expected "
								+ SyntaxVariable::name(type));
				e.where = option->pos;
				throw e;
			}
			return v;
		}
	}

	const SyntaxNode::Var *SyntaxNode::find(const String &name) const {
		VarMap::const_iterator i = vars.find(name);
		if (i == vars.end())
			return null;
		else
			return &i->second;
	}

}
