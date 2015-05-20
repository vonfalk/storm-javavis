#include "stdafx.h"
#include "SyntaxTransform.h"
#include "Code/FnParams.h"
#include "Function.h"
#include "Parser.h"

#include "Lib/Debug.h"

namespace storm {

	// Lookup a name.
	Type *lookupType(TransformEnv &env, const SrcPos &pos, const Scope &scope, const String &name) {
		Auto<Name> n = parseTemplateName(env.e, scope, pos, name);
		return as<Type>(scope.find(n));
	}

	ActualObject::ActualObject(const Auto<Object> &v, const SrcPos &pos) : v(v) {
		if (!v)
			throw SyntaxTypeError(pos, L"Null is not an allowed return value.");
	}

	ActualObject::~ActualObject() {}

	/**
	 * Evaluation
	 */

	static Auto<ActualBase> transform(TransformEnv &env, const SyntaxNode &n,
									const vector<Auto<ActualBase>> &params, const SrcPos *pos);

	// Evaluate a syntax variable.
	static Auto<ActualBase> evaluate(TransformEnv &env, SyntaxVariable *v, const vector<Auto<ActualBase>> &params) {
		switch (v->type) {
		case SyntaxVariable::tString:
		{
			SStr *result = CREATE(SStr, env.e, v->string());
			result->pos = v->pos;
			return new ActualObject(result, v->pos);
		}
		case SyntaxVariable::tNode:
			return transform(env, *v->node(), params, &v->pos);
			break;
		default:
			assert(false);
			return null;
		}
	}

	static vector<Value> types(const vector<Auto<ActualBase>> &p) {
		vector<Value> r(p.size());
		for (nat i = 0; i < p.size(); i++)
			r[i] = p[i]->type();
		return r;
	}

	static vector<Value> types(Type *first, const vector<Auto<ActualBase>> &p) {
		vector<Value> r(p.size() + 1);
		r[0] = Value::thisPtr(first);
		for (nat i = 0; i < p.size(); i++)
			r[i + 1] = p[i]->type();
		return r;
	}

	static Auto<ActualBase> tryCallFn(const SyntaxOption *option, Auto<Name> find, const vector<Auto<ActualBase>> &p) {
		vector<Value> t = types(p);
		find = find->withParams(t);
		Function *f = as<Function>(option->scope.find(find));
		if (!f)
			return Auto<ActualBase>(null);

		if (f->result == Value())
			throw SyntaxTypeError(option->pos, L"A rule's function must return a value.");
		if (!f->result.isClass())
			throw SyntaxTypeError(option->pos, L"Only objects are supported in the syntax. "
								+ ::toS(f->result) + L" is a value or a built-in type.");

		code::FnParams params;
		for (nat i = 0; i < p.size(); i++)
			p[i]->add(params);
		return new ActualObject(f->call<Object *>(params), option->pos);
	}

	static Auto<ActualBase> tryCallCtor(const SyntaxOption *option, Auto<Name> find, const vector<Auto<ActualBase>> &p) {
		Type *t = as<Type>(option->scope.find(find));
		if (!t)
			return Auto<ActualBase>(null);

		if ((t->flags & typeClass) == 0)
			throw SyntaxTypeError(option->pos, L"Only objects are supported in the syntax. "
								+ ::toS(find) + L" is a value or a built-in type.");

		Function *ctor = as<Function>(t->findCpp(Type::CTOR, types(t, p)));
		if (!ctor)
			return Auto<ActualBase>(null);

		code::FnParams params;
		for (nat i = 0; i < p.size(); i++)
			p[i]->add(params);

		return new ActualObject(create<Object>(ctor, params), option->pos);
	}

	static Auto<ActualBase> callFn(TransformEnv &env, const SyntaxOption *option, const vector<Auto<ActualBase>> &p) {
		Auto<ActualBase> result;
		Auto<Name> name = parseTemplateName(env.e, option->scope, option->pos, option->matchFn);
		Auto<NamePart> last = name->last();
		if (last->params.size() == 0)
			if (result = tryCallFn(option, name, p))
				return result;

		if (result = tryCallCtor(option, name, p))
			return result;

		throw SyntaxTypeError(option->pos, L"Could not find a function or constructor " + ::toS(name)
							+ L"(" + join(types(p), L", ") + L").");
	}

	static void callMember(const String &name, Auto<ActualBase> me, Auto<ActualBase> param, const SrcPos &pos) {
		Type *t = me->type().type;
		vector<Value> types(2);
		types[0] = me->type();
		types[1] = param->type();

		if (Function *f = as<Function>(t->findCpp(name, types))) {
			code::FnParams call;
			me->add(call);
			param->add(call);

			if (f->result.refcounted())
				Auto<Object> o = f->call<Object *>(call);
			else if (f->result == Value())
				f->call<void>(call);
			else
				throw SyntaxTypeError(pos, L"Member functions called from the syntax may only return void or Objects.");
		} else {
			throw SyntaxTypeError(pos, L"Could not find a member function " + name
								+ L"(" + join(types, L", ") + L") in " + t->identifier());
		}
	}

	static Auto<ActualBase> transformFn(SyntaxVars &vars, TransformEnv &env, const SyntaxNode &n, const SrcPos *pos) {
		const SyntaxOption *option = n.option;
		const vector<String> &names = option->matchFnParams;
		vector<Auto<ActualBase>> params(names.size());
		for (nat i = 0; i < names.size(); i++)
			params[i] = vars.get(names[i]);

		// Call it.
		Auto<ActualBase> r = callFn(env, option, params);

		// Set the 'pos' member if it exists.
		if (pos)
			if (ActualObject *o = as<ActualObject>(r.borrow()))
				if (SObject *s = as<SObject>(o->v.borrow()))
					s->pos = *pos;

		// Add the 'me' variable. It may be used by later rules.
		vars.set(L"me", r);

		// Call any member functions...
		for (nat i = 0; i < n.invocations.size(); i++) {
			const SyntaxNode::Invocation &v = n.invocations[i];
			vector<Auto<ActualBase>> params(v.val.params.size());
			for (nat i = 0; i < v.val.params.size(); i++)
				params[i] = vars.get(v.val.params[i]);

			Auto<ActualBase> tmp = evaluate(env, v.val.value, params);
			callMember(v.member, r, tmp, option->pos);
		}

		return r;
	}

	static Auto<ActualBase> transform(TransformEnv &env, const SyntaxNode &n,
									const vector<Auto<ActualBase>> &params, const SrcPos *pos) {
		const SyntaxOption *option = n.option;
		SyntaxVars vars(n, params, env, pos);

		if (option->matchVar)
			return vars.get(option->matchFn);
		else
			return transformFn(vars, env, n, pos);
	}


	Auto<Object> transform(Engine &e,
					SyntaxSet &syntax,
					const SyntaxNode &node,
					const vector<Object*> &params,
					const SrcPos *pos) {
		TransformEnv env = { e, syntax };
		vector<Auto<ActualBase>> p(params.size());
		for (nat i = 0; i < params.size(); i++) {
			params[i]->addRef();
			p[i] = new ActualObject(params[i], SrcPos());
		}

		Auto<ActualBase> result = transform(env, node, p, pos);
		if (ActualObject *o = as<ActualObject>(result.borrow()))
			return o->v;

		throw SyntaxTypeError(SrcPos(), L"The root rule has to return an Object.");
	}




	/**
	 * SyntaxVars
	 */

	SyntaxVars::SyntaxVars(const SyntaxNode &node, const vector<Auto<ActualBase>> &params,
						TransformEnv &env, const SrcPos *pos)
		: node(node), env(env), pos(pos) {
		addParams(params);
	}

	SyntaxVars::~SyntaxVars() {}

	void SyntaxVars::addParams(vector<Auto<ActualBase>> params) {
		const SyntaxOption *option = node.option;
		SyntaxRule *rule = env.syntax.rule(option->rule());

		if (params.size() != rule->params.size())
			throw SyntaxTypeError(option->pos, L"Invalid number of parameters to " + rule->name()
								+ L": got " + ::toS(params.size()) + L", expected "
								+ ::toS(rule->params.size()));

		for (nat i = 0; i < params.size(); i++) {
			const SyntaxRule::Param &p = rule->params[i];
			Type *t = lookupType(env, rule->declared, rule->declScope, p.type);
			if (t == null)
				throw SyntaxTypeError(rule->declared, L"Unknown type: " + p.type);
			Value(t).mustStore(params[i]->type(), option->pos);

			vars.insert(make_pair(p.name, params[i]));
		}
	}

	void SyntaxVars::set(const String &name, Auto<ActualBase> v) {
		Map::iterator i = vars.find(name);
		if (i != vars.end()) {
			delete v;
			throw SyntaxTypeError(node.option->pos, L"The variable " + name + L" is already set!");
		}
		vars.insert(make_pair(name, v));
	}

	Auto<ActualBase> SyntaxVars::get(const String &name) {
		Map::iterator i = vars.find(name);
		if (i == vars.end())
			return eval(name);

		if (!i->second)
			throw SyntaxTypeError(node.option->pos, L"The variable " + name + L" depends on itself!");

		return i->second;
	}

	Auto<ActualBase> SyntaxVars::eval(const String &name) {
		const SyntaxNode::Var *v = node.find(name);
		if (v == null)
			return valueOf(name);

		vars.insert(make_pair(name, Auto<ActualBase>(null)));

		vector<Auto<ActualBase>> params(v->params.size());
		for (nat i = 0; i < v->params.size(); i++)
			params[i] = get(v->params[i]);

		Auto<ActualBase> r = evaluate(env, v->value, params);
		vars[name] = r;
		return r;
	}

	Auto<ActualBase> SyntaxVars::valueOf(const String &v) {
		if (v == L"pos") {
			// Create 'pos' only where it is needed.
			if (pos == null)
				return new Actual<SrcPos>(SrcPos(), SrcPos::stormType(env.e));
			else
				return new Actual<SrcPos>(*pos, SrcPos::stormType(env.e));
		}

		if (v.isInt())
			return new Actual<Int>(v.toInt(), intType(env.e));

		throw SyntaxTypeError(node.option->pos, L"The variable " + v + L" is not defined.");
	}

}
