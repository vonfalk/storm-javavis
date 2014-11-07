#include "stdafx.h"
#include "SyntaxTransform.h"
#include "Code/Function.h"
#include "Function.h"

namespace storm {

	// Evaluate a syntaxVariable. Never returns null.
	static Object *evaluate(Engine &engine, SyntaxSet &syntax, SyntaxVariable *v, const vector<Object *> &params) {
		switch (v->type) {
		case SyntaxVariable::tString:
			return new Str(engine, v->string());
		case SyntaxVariable::tNode:
			return transform(engine, syntax, *v->node(), params);
		case SyntaxVariable::tStringArr:
		case SyntaxVariable::tNodeArr:
			TODO(L"Arrays not supported yet!");
			return null;
		}

		throw SyntaxTypeError(L"Unknown type of syntax variable.");
	}

	// Call a function.
	static Object *callFunction(Engine &e, const SyntaxOption *option, const vector<Object *> &params) {
		vector<Value> types(params.size());
		for (nat i = 0; i < params.size(); i++)
			types[i] = Value(params[i]->myType);

		NameOverload *no = option->scope.find(option->matchFn, types);
		if (Function *f = as<Function>(no)) {
			code::FnCall call;
			for (nat i = 0; i < params.size(); i++)
			 	call.param(params[i]);
			return call.call<Object *>(f->pointer());
		}

		// See if we can find a constructor!
		if (Type *t = as<Type>(option->scope.find(option->matchFn))) {
			types.insert(types.begin(), Value(e.typeType()));
			no = Scope(t).find(Type::CTOR, types);
			if (Function *ctor = as<Function>(no)) {
				code::FnCall call;
				call.param(t);
				for (nat i = 0; i < params.size(); i++)
					call.param(t);
				return call.call<Object *>(ctor->pointer());
			}

			throw SyntaxTypeError(L"Could not find a constructor " +
								toS(option->matchFn) + L"(" + join(types, L", ") + L")",
								option->pos);
		}

		throw SyntaxTypeError(L"Could not find a function " +
							toS(option->matchFn) + L"(" + join(types, L", ") + L")",
							option->pos);
	}

	// Call a member function.
	static Object *callMember(Object *me, const String &memberName, Object *param, const SrcPos &pos) {
		if (me == null || param == null)
			throw SyntaxTypeError(L"Null is not supported!", pos);

		Type *t = me->myType;
		vector<Value> types(2);
		types[0] = Value(t);
		types[1] = Value(param->myType);
		NameOverload *no = Scope(t).find(Name(memberName), types);
		if (Function *f = as<Function>(no)) {
			code::FnCall call;
			call.param(me).param(param);
			return call.call<Object *>(f->pointer());
		}

		throw SyntaxTypeError(L"Could not find a member function " +
							memberName + L"(" + join(types, L", ") + L") in " +
							t->name, pos);
	}


	Object *transform(Engine &e, SyntaxSet &syntax, const SyntaxNode &node, const vector<Object*> &params) {
		const SyntaxOption *option = node.option;
		Object *result = null;
		Object *tmp = null;
		SyntaxVars vars(e, syntax, node, params);

		try {
			vector<Object*> values;
			for (nat i = 0; i < option->matchFnParams.size(); i++) {
				const String &p = option->matchFnParams[i];
				values.push_back(vars.get(p));
			}

			result = callFunction(e, option, values);
			if (result == null)
				throw SyntaxTypeError(L"Syntax generating functions may not return null.", option->pos);
			vars.set(L"me", result);

			// Call any remaining member functions...
			for (nat i = 0; i < node.invocations.size(); i++) {
				const SyntaxNode::Invocation &v = node.invocations[i];
				vector<Object*> params(v.val.params.size());
				for (nat i = 0; i < v.val.params.size(); i++) {
					params[i] = vars.get(v.val.params[i]);
				}

				tmp = evaluate(e, syntax, v.val.value, params);
				callMember(result, v.member, tmp, option->pos);
				release(tmp);
			}

		} catch (...) {
			release(result);
			release(tmp);
			throw;
		}

		return result;
	}


	/**
	 * The SyntaxVars type.
	 */

	SyntaxVars::SyntaxVars(Engine &e, SyntaxSet &syntax, const SyntaxNode &node, const vector<Object*> &params)
		: e(e), syntax(syntax), node(node) {

		const SyntaxOption *option = node.option;
		const SrcPos &pos = node.option->pos;
		SyntaxRule *rule = syntax.rule(option->rule());
		assert(rule);

		if (params.size() != rule->params.size())
			throw SyntaxTypeError(L"Invalid number of parameters to rule: " + toS(params.size()) +
								L", expected " + toS(rule->params.size()), pos);

		for (nat i = 0; i < params.size(); i++) {
			const SyntaxRule::Param &param = rule->params[i];
			Type *t = as<Type>(option->scope.find(Name(param.type)));
			if (t == null)
				throw SyntaxTypeError(L"Unknown type: " + param.type);
			if (params[i] == null)
				throw SyntaxTypeError(L"Null is not supported!");

			Type *vt = params[i]->myType;
			if (!Value(t).canStore(vt))
				throw SyntaxTypeError(L"Incompatible types: got " + vt->name + L", expected " + t->name, pos);
			vars.insert(make_pair(param.name, params[i]));
			params[i]->addRef();
		}
	}

	SyntaxVars::~SyntaxVars() {
		releaseMap(vars);
	}


	Object *SyntaxVars::get(const String &name) {
		Map::iterator i = vars.find(name);
		if (i != vars.end())
			return i->second;

		if (currentNames.count(name) != 0)
			throw SyntaxTypeError(L"Variable " + name + L" depends on itself!", node.option->pos);
		currentNames.insert(name);

		try {
			const SyntaxNode::Var *v = node.find(name);
			if (v == null) {
				TODO(L"This may happen if an array should contain zero elements...");
				throw SyntaxTypeError(L"The variable " + name + L" does not exist!", node.option->pos);
			}

			vector<Object*> params(v->params.size());
			for (nat i = 0; i < v->params.size(); i++)
				params[i] = get(v->params[i]);

			Object *o = evaluate(e, syntax, v->value, params);
			vars.insert(make_pair(name, o));
			currentNames.erase(name);
			return o;
		} catch (...) {
			currentNames.erase(name);
			throw;
		}
	}

	void SyntaxVars::set(const String &name, Object *o) {
		if (vars.count(name) == 0) {
			vars.insert(make_pair(name, o));
			o->addRef();
		} else {
			throw SyntaxTypeError(L"The variable " + name + L" is already set!", node.option->pos);
		}
	}

}
