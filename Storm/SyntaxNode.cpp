#include "stdafx.h"
#include "SyntaxNode.h"
#include "Code/Function.h"
#include "Function.h"

namespace storm {

	SyntaxNode::SyntaxNode(const SyntaxOption *option) : option(option) {}

	SyntaxNode::~SyntaxNode() {
		clearMap(vars);
		clearMap(invocations); // Hack!
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

	void SyntaxNode::add(const String &var, const String &val) {
		try {
			SyntaxVariable *v = find(var, typeOf(var, true));
			v->add(val);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::add(const String &var, SyntaxNode *node) {
		try {
			SyntaxVariable *v = find(var, typeOf(var, false));
			v->add(node);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::invoke(const String &m, const String &val) {
		try {
			SyntaxVariable *sv = new SyntaxVariable(SyntaxVariable::tString);
			invocations.push_back(make_pair(m, sv));
			sv->add(val);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
			throw e;
		}
	}

	void SyntaxNode::invoke(const String &m, SyntaxNode *val) {
		try {
			SyntaxVariable *sv = new SyntaxVariable(SyntaxVariable::tNode);
			invocations.push_back(make_pair(m, sv));
			sv->add(val);
		} catch (SyntaxTypeError e) {
			e.where = option->pos;
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
			if (v->type != type) {
				SyntaxTypeError e(L"Invalid type of " + name + L": "
								+ SyntaxVariable::name(v->type)
								+ L", expected "
								+ SyntaxVariable::name(type));
				e.where = option->pos;
				throw e;
			}
			return v;
		}
	}

	SyntaxVariable *SyntaxNode::find(const String &name) const {
		VarMap::const_iterator i = vars.find(name);
		if (i == vars.end())
			return null;
		else
			return i->second;
	}

	static Object *evaluate(Engine &engine, SyntaxVariable *v) {
		switch (v->type) {
		case SyntaxVariable::tString:
			return new Str(engine.strType(), v->string());
		case SyntaxVariable::tNode:
			return transform(engine, *v->node());
		case SyntaxVariable::tStringArr:
		case SyntaxVariable::tNodeArr:
			TODO(L"Arrays not supported yet!");
			return null;
		}

		return null;
	}

	static Object *evaluate(Engine &engine, const SyntaxNode &node, const String &var) {
		SyntaxVariable *v = node.find(var);
		if (v == null)
			throw SyntaxTypeError(L"The variable " + var + L" does not exist!", node.option->pos);

		return evaluate(engine, v);
	}

	static Object *callFunction(Engine &e, const SyntaxOption *option, const vector<Object *> &params) {
		vector<Value> types(params.size());
		for (nat i = 0; i < params.size(); i++)
			types[i] = Value(params[i]->type);

		NameOverload *no = option->scope->find(option->matchFn, types);
		if (Function *f = as<Function>(no))
			return code::fnCall<Object, Object>(f->pointer(), params);

		// See if we can find a constructor!
		if (Type *t = as<Type>(option->scope->find(option->matchFn))) {
			types.insert(types.begin(), Value(e.typeType()));
			no = t->find(Type::CTOR, types);
			if (Function *ctor = as<Function>(no)) {
				vector<void *> p(params.size() + 1);
				p[0] = t;
				for (nat i = 0; i < params.size(); i++)
					p[i+1] = params[i];
				return code::fnCall<Object, void>(ctor->pointer(), p);
			}

			throw SyntaxTypeError(L"Could not find a constructor " +
								toS(option->matchFn) + L"(" + join(types, L", ") + L")",
								option->pos);
		}

		throw SyntaxTypeError(L"Could not find a function " +
							toS(option->matchFn) + L"(" + join(types, L", ") + L")",
							option->pos);
	}

	static Object *callMember(Object *me, const String &memberName, Object *param, const SrcPos &pos) {
		if (me == null || param == null)
			throw SyntaxTypeError(L"Null is not supported!", pos);

		vector<Value> types(1, Value(param->type));
		Type *t = me->type;
		NameOverload *no = t->find(Name(memberName), types);
		if (Function *f = as<Function>(no)) {
			vector<Object*> params(1, param);
			return code::fnCall<Object, Object>(f->pointer(), params);
		}

		throw SyntaxTypeError(L"Could not find a member function " +
							memberName + L"(" + join(types, L", ") + L") in" +
							t->name, pos);
	}

	Object *transform(Engine &e, const SyntaxNode &node) {
		hash_map<String, Object*> params;
		const SyntaxOption *option = node.option;
		Object *result = null;
		Object *tmp = null;

		try {
			for (nat i = 0; i < option->matchFnParams.size(); i++) {
				const String &p = option->matchFnParams[i];
				if (params.count(p) == 0)
					params.insert(make_pair(p, evaluate(e, node, p)));
			}

			vector<Object*> values;
			for (nat i = 0; i < option->matchFnParams.size(); i++) {
				const String &p = option->matchFnParams[i];
				Object *value = params[p];
				if (value == null)
					throw SyntaxTypeError(L"Null is not allowed!", option->pos);
				values.push_back(value);
			}

			result = callFunction(e, option, values);

			// Call any remaining member functions...
			for (nat i = 0; i < node.invocationCount(); i++) {
				std::pair<String, SyntaxVariable*> v = node.invocation(i);
				tmp = evaluate(e, v.second);
				callMember(result, v.first, tmp, option->pos);
				tmp = null;
			}

		} catch (...) {
			releaseMap(params);
			release(result);
			release(tmp);
			throw;
		}
		releaseMap(params);
		return result;
	}

}
