#include "stdafx.h"
#include "SyntaxNode.h"

namespace storm {

	SyntaxNode::SyntaxNode(const SyntaxOption *option)
		: option(option), invocations(mInvocations) {}

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

	void SyntaxNode::add(const String &var, const String &val, const vector<String> &params, const SrcPos &pos) {
		if (vars.count(var))
			throw SyntaxVarAssignedError(pos, var);

		Var v = { new SyntaxVariable(pos, val), params };
		vars.insert(make_pair(var, v));
	}

	void SyntaxNode::add(const String &var, SyntaxNode *val, const vector<String> &params, const SrcPos &pos) {
		if (vars.count(var))
			throw SyntaxVarAssignedError(pos, var);

		Var v = { new SyntaxVariable(pos, val), params };
		vars.insert(make_pair(var, v));
	}

	void SyntaxNode::invoke(const String &m, const String &val, const vector<String> &params, const SrcPos &pos) {
		Invocation i = { m, { new SyntaxVariable(pos, val), params } };
		mInvocations.push_back(i);
	}

	void SyntaxNode::invoke(const String &m, SyntaxNode *val, const vector<String> &params, const SrcPos &pos) {
		Invocation i = { m, { new SyntaxVariable(pos, val), params } };
		mInvocations.push_back(i);
	}

	void SyntaxNode::reverseArrays() {
		std::reverse(mInvocations.begin(), mInvocations.end());
	}

	const SyntaxNode::Var *SyntaxNode::find(const String &name) const {
		VarMap::const_iterator i = vars.find(name);
		if (i == vars.end())
			return null;
		else
			return &i->second;
	}

}
