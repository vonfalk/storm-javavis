#include "stdafx.h"
#include "BSCtor.h"
#include "Parser.h"
#include "BSScope.h"
#include "BSNamed.h"

namespace storm {

	bs::BSCtor::BSCtor(const vector<Value> &values, const vector<String> &names,
				const Scope &scope, Par<SStr> contents, const SrcPos &pos)
		: Function(Value(), Type::CTOR, values), scope(scope), contents(contents), paramNames(names), pos(pos) {

		// Rename until it is initialized!
		paramNames[0] = L" this";

		setCode(steal(CREATE(LazyCode, this, memberVoidFn(this, &BSCtor::generateCode))));
	}

	code::Listing bs::BSCtor::generateCode() {
		SyntaxSet syntax;
		addSyntax(scope, syntax);

		Parser parser(syntax, contents->v->v, contents->pos);
		nat parsed = parser.parse(L"CtorBody");
		if (parser.hasError())
			throw parser.error();

		Auto<Object> c = parser.transform(engine(), vector<Object *>(1, this));
		Auto<CtorBody> body = c.expect<CtorBody>(engine(), L"While evaluating CtorBody");

		using namespace code;
		Listing l;
		CodeData data;

		l << prolog();

		// Parameters (first one is special).
		{
			Variable thisVar = l.frame.createParameter(params[0].size(), false);
			LocalVar *hidden = body->variable(L" this");
			LocalVar *normal = body->variable(L"this");
			hidden->var = thisVar;
			normal->var = thisVar;
		}

		GenState state = { l, data, runOn(), l.frame, l.frame.root() };

		for (nat i = 1; i < params.size(); i++) {
			const Value &t = params[i];
			LocalVar *var = body->variable(paramNames[i]);
			assert(var);
			var->createParam(state);
		}

		GenResult r;
		body->code(state, r);

		l << epilog();
		l << ret(Size());
		l << data;

		// PLN(identifier() << L": " << l);
		return l;
	}

	void bs::BSCtor::addParams(Par<Block> to) {
		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);
			if (i == 0)
				var->constant = true;
			to->add(var);
		}
	}

	bs::CtorBody::CtorBody(Par<BSCtor> ctor) : ExprBlock(ctor->scope) {
		ctor->addParams(this);
	}

	void bs::CtorBody::add(Par<ArrayP<Expr>> exprs) {
		for (nat i = 0; i < exprs->count(); i++) {
			add(exprs->at(i));
		}
	}

	bs::Initializer::Initializer(Par<SStr> name, Par<Expr> expr) : name(name) {
		params = CREATE(Actual, this);
		params->add(expr);
	}

	bs::Initializer::Initializer(Par<SStr> name, Par<Actual> params) : name(name), params(params) {}

	bs::SuperCall::SuperCall(Par<CtorBody> block, Par<Actual> params, Par<ArrayP<Initializer>> init) {
		this->init(block, params);
		for (nat i = 0; i < init->count(); i++)
			this->init(init->at(i));
	}

	bs::SuperCall::SuperCall(Par<CtorBody> block, Par<Actual> params) {
		init(block, params);
	}

	void bs::SuperCall::init(Par<CtorBody> block, Par<Actual> params) {
		// Add the regular this parameter!
		thisVar = capture(block->variable(L" this"));
		thisPtr = thisVar->result;
		Auto<LocalVar> created = CREATE(LocalVar, this, L"this", thisPtr, thisVar->pos, true);
		created->constant = true;
		block->add(created);

		this->params = params;
		Auto<LocalVarAccess> l = CREATE(LocalVarAccess, this, thisVar);
		params->addFirst(l);

		scope = block->scope;
	}

	Value bs::SuperCall::result() {
		return Value();
	}

	void bs::SuperCall::init(Par<Initializer> init) {
		const String &name = init->name->v->v;
		TypeVar *v = as<TypeVar>(thisPtr.type->find(name, vector<Value>(1, thisPtr)));
		if (v == null || v->params[0].type != thisPtr.type)
			throw SyntaxError(init->name->pos, L"The member variable " + name + L" was not found in "
							+ ::toS(thisPtr));

		if (initMap.count(name))
			throw SyntaxError(init->name->pos, L"The member " + name + L" has already been initialized.");

		initMap.insert(make_pair(name, init->params));
	}

	void bs::SuperCall::callParent(GenState &s) {
		Type *parent = thisPtr.type->super();
		if (!parent)
			return;

		// Find something to call.
		vector<Value> values = params->values();
		values[0].type = parent;
		Function *ctor = as<Function>(parent->find(Type::CTOR, values));
		if (!ctor)
			throw SyntaxError(pos, L"No constructor (" + join(values, L", ") + L") found in " + parent->identifier());

		vector<code::Value> actuals(values.size());
		for (nat i = 0; i < values.size(); i++)
			actuals[i] = params->code(i, s, ctor->params[i]);

		GenResult t;
		ctor->localCall(s, actuals, t, false);
	}

	void bs::SuperCall::code(GenState &s, GenResult &r) {
		using namespace code;

		// Super class should be called first.
		callParent(s);

		Variable dest = thisVar->var;
		Type *type = thisPtr.type;

		// Set our VTable.
		if (type->flags & typeClass) {
			// TODO: maybe symbolic offset here?
			s.to << mov(ptrA, dest);
			s.to << mov(ptrRel(ptrA), type->vtable.ref);
		}

		// Initialize any member variables.
		vector<Auto<TypeVar>> vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			initVar(s, vars[i]);
		}
	}

	void bs::SuperCall::initVar(GenState &s, Par<TypeVar> v) {
		InitMap::iterator i = initMap.find(v->name);
		if (i == initMap.end())
			initVarDefault(s, v);
		else
			initVar(s, v, i->second);
	}

	void bs::SuperCall::initVarDefault(GenState &s, Par<TypeVar> v) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var;

		if (t.ref)
			throw SyntaxError(pos, L"Can not initialize reference " + v->name + L", not implemented yet!");

		if (t.isValue()) {
			s.to << mov(ptrA, dest);
			s.to << add(ptrA, intPtrConst(v->offset()));
			s.to << fnParam(ptrA);
			s.to << fnCall(t.defaultCtor(), Size());
		} else {
			// Initialize classes to default as well?
		}
	}

	void bs::SuperCall::initVar(GenState &s, Par<TypeVar> v, Par<Actual> to) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var;

		if (t.ref)
			throw SyntaxError(pos, L"Can not initialize reference " + v->name + L", not implemented yet!");

		Type *toCreate = t.type;
		vector<Value> values = to->values();
		values.insert(values.begin(), Value::thisPtr(toCreate));
		Function *ctor = as<Function>(toCreate->find(Type::CTOR, values));
		if (ctor == null)
			throw SyntaxError(to->pos, L"No constructor for " + ::toS(t) + L"(" + join(values, L", ") + L").");

		if (t.isClass()) {
			// Easy way, call the constructor as normal.
			Auto<CtorCall> call = CREATE(CtorCall, this, ctor, to);
			GenResult created(t, s.part);
			call->code(s, created);
			Variable loc = created.location(s);
			s.to << mov(ptrA, dest);
			s.to << mov(ptrRel(ptrA, v->offset()), loc);
			s.to << code::addRef(loc);
			return;
		}

		// Now we're left with the values!

		vector<code::Value> actuals(values.size());
		for (nat i = 1; i < values.size(); i++)
			actuals[i] = to->code(i - 1, s, ctor->params[i]);

		s.to << mov(ptrA, dest);
		s.to << add(ptrA, intPtrConst(v->offset()));
		actuals[0] = ptrA;

		GenResult nothing;
		ctor->localCall(s, actuals, nothing, true);
	}

}
