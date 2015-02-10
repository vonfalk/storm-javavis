#include "stdafx.h"
#include "SyntaxTransform.h"
#include "Code/Function.h"
#include "Function.h"
#include "Parser.h"
#include "Tokenizer.h"

#include "Lib/Debug.h"

namespace storm {

	// Evaluate a syntaxVariable. Never returns null.
	static Object *evaluate(Engine &engine, SyntaxSet &syntax, SyntaxVariable *v, const vector<Object *> &params) {
		switch (v->type) {
		case SyntaxVariable::tString: {
			SStr *result = CREATE(SStr, engine, v->string());
			result->pos = v->pos;
			return result;
		}
		case SyntaxVariable::tNode:
			return transform(engine, syntax, *v->node(), params, &v->pos);
		default:
			assert(false);
			return null;
		}
	}

	// Parse a string into a Name, supporting <> as template parameters.
	static Name *parseName(Engine &e, const Scope &scope, const SrcPos &pos, Tokenizer &tok);

	static NamePart *parseNamePart(Engine &e, const Scope &scope, const SrcPos &pos, Tokenizer &tok) {
		String name = tok.next().token;
		vector<Value> params;

		if (tok.more() && tok.peek().token == L"<") {
			tok.next();

			while (tok.more() && tok.peek().token != L">") {
				Auto<Name> n = parseName(e, scope, pos, tok);
				if (Type *t = as<Type>(scope.find(n)))
					params.push_back(Value(t));
				else
					throw SyntaxError(pos, L"Unknown type: " + ::toS(n));

				if (!tok.more())
					throw SyntaxError(pos, L"Unbalanced <>");
				Token t = tok.peek();
				if (t.token == L">")
					break;
				if (t.token != L",")
					throw SyntaxError(pos, L"Expected , got" + t.token);
			}

			tok.next();
		}

		return CREATE(NamePart, e, name, params);
	}

	static Name *parseName(Engine &e, const Scope &scope, const SrcPos &pos, Tokenizer &tok) {
		Auto<Name> r = CREATE(Name, e);
		Auto<NamePart> part;

		while (tok.more()) {
			Auto<NamePart> part = parseNamePart(e, scope, pos, tok);
			r->add(part);

			if (!tok.more())
				break;

			Token t = tok.peek();
			if (t.token == L">")
				break;
			if (t.token == L",")
				break;
			if (t.token != L".")
				throw SyntaxError(pos, L"Expected . got " + t.token);
			tok.next();
		}

		return r.ret();
	}

	static Name *parseName(Engine &e, const Scope &scope, const SrcPos &pos, const String &src) {
		Tokenizer tok(Path(), src, 0);
		return parseName(e, scope, pos, tok);
	}

	// Call a function.
	static Object *callFunction(Engine &e, const SyntaxOption *option, const vector<Object *> &params, const SrcPos &pos) {
		vector<Value> types(params.size());
		for (nat i = 0; i < params.size(); i++) {
			if (params[i])
				types[i] = Value(params[i]->myType);
			else
				types[i] = SrcPos::type(e);
		}

		Auto<Name> name = parseName(e, option->scope, option->pos, option->matchFn);

		if (name->at(name->size() - 1)->params.size() == 0) {
			Auto<Name> match = name->withParams(types);
			Named *no = option->scope.find(match);
			if (Function *f = as<Function>(no)) {
				if (f->result == Value())
					throw SyntaxTypeError(option->pos, L"A rule's function must return a value.");
				if (!f->result.isClass())
					throw SyntaxTypeError(option->pos, L"Only objects are supported in the syntax. "
										+ ::toS(f->result) + L" is a value or a built-in type.");

				code::FnCall call;
				for (nat i = 0; i < params.size(); i++) {
					if (params[i]) {
						call.param<Object *>(params[i]);
					} else {
						call.param<SrcPos>(pos);
					}
				}
				return call.call<Object *>(f->pointer());
			}
		}

		// See if we can find a constructor!
		if (Type *t = as<Type>(option->scope.find(name))) {
			types.insert(types.begin(), Value(t));
			Auto<NamePart> ctor = CREATE(NamePart, e, Type::CTOR, types);
			Named *no = t->find(ctor);
			if (Function *ctor = as<Function>(no)) {
				code::FnCall call;
				for (nat i = 0; i < params.size(); i++) {
					if (params[i]) {
						call.param<Object *>(params[i]);
					} else {
						call.param<SrcPos>(pos);
					}
				}
				return create<Object>(ctor, call);
			}

			throw SyntaxTypeError(option->pos, L"Could not find a constructor " +
								toS(option->matchFn) + L"(" + join(types, L", ") + L")");
		}

		throw SyntaxTypeError(option->pos, L"Could not find a function " +
							toS(option->matchFn) + L"(" + join(types, L", ") + L")");
	}

	// Call a member function. NOTE: No support for param==null -> pos!
	static void callMember(Object *me, const String &memberName, Object *param, const SrcPos &pos) {
		if (me == null || param == null)
			throw SyntaxTypeError(pos, L"Null is not supported!");

		Type *t = me->myType;
		vector<Value> types(2);
		types[0] = Value(t);
		types[1] = Value(param->myType);
		Auto<NamePart> part = CREATE(NamePart, t, memberName, types);

		if (Function *f = as<Function>(t->find(part))) {
			code::FnCall call;
			call.param(me).param(param);

			if (f->result.refcounted())
				Auto<Object> r = call.call<Object*>(f->pointer());
			else
				call.call<void>(f->pointer());
		} else {
			throw SyntaxTypeError(pos, L"Could not find a member function " +
								memberName + L"(" + join(types, L", ") + L") in " +
								t->name);
		}
	}


	Object *transform(Engine &e,
					SyntaxSet &syntax,
					const SyntaxNode &node,
					const vector<Object*> &params,
					const SrcPos *pos)
	{
		const SyntaxOption *option = node.option;
		Auto<Object> result;
		SyntaxVars vars(e, syntax, node, params);
		SrcPos posCopy;
		if (pos) posCopy = *pos;

		if (option->matchVar) {
			Object *t = vars.get(option->matchFn);
			t->addRef();
			return t;
		}

		vector<Object*> values; // borrowed ptrs.
		for (nat i = 0; i < option->matchFnParams.size(); i++) {
			const String &p = option->matchFnParams[i];
			values.push_back(vars.get(p));
		}

		result = callFunction(e, option, values, posCopy);
		if (result == null)
			throw SyntaxTypeError(option->pos, L"Syntax generating functions may not return null.");

		if (SObject *s = as<SObject>(result.borrow()))
			if (pos)
				s->pos = *pos;
		vars.set(L"me", result.borrow());

		// Call any remaining member functions...
		for (nat i = 0; i < node.invocations.size(); i++) {
			const SyntaxNode::Invocation &v = node.invocations[i];
			vector<Object*> params(v.val.params.size());
			for (nat i = 0; i < v.val.params.size(); i++) {
				params[i] = vars.get(v.val.params[i]);
			}

			Auto<Object> tmp = evaluate(e, syntax, v.val.value, params);
			callMember(result.borrow(), v.member, tmp.borrow(), option->pos);
		}

		return result.ret();
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
			throw SyntaxTypeError(pos, L"Invalid number of parameters to rule " + rule->name()
								+ L": got " + toS(params.size()) +
								L", expected " + toS(rule->params.size()));

		for (nat i = 0; i < params.size(); i++) {
			const SyntaxRule::Param &param = rule->params[i];
			Auto<Name> typeName = parseSimpleName(e, param.type);
			Type *t = as<Type>(option->scope.find(typeName));
			if (t == null)
				throw SyntaxTypeError(pos, L"Unknown type: " + param.type);
			if (params[i] == null)
				throw SyntaxTypeError(pos, L"Null is not supported!");

			Type *vt = params[i]->myType;
			if (!Value(t).canStore(vt))
				throw SyntaxTypeError(pos, L"Incompatible types: got " + vt->name + L", expected " + t->name);
			vars.insert(make_pair(param.name, params[i]));
			params[i]->addRef();
		}
	}

	SyntaxVars::~SyntaxVars() {
		releaseMap(vars);
	}


	Object *SyntaxVars::get(const String &name) {
		if (name == L"pos")
			return null;

		Map::iterator i = vars.find(name);
		if (i != vars.end())
			return i->second;

		if (currentNames.count(name) != 0)
			throw SyntaxTypeError(node.option->pos, L"Variable " + name + L" depends on itself!");
		currentNames.insert(name);

		try {
			const SyntaxNode::Var *v = node.find(name);
			if (v == null)
				throw SyntaxTypeError(node.option->pos, L"The variable " + name + L" does not exist!");

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
			throw SyntaxTypeError(node.option->pos, L"The variable " + name + L" is already set!");
		}
	}

}
