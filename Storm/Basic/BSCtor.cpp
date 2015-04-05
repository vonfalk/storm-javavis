#include "stdafx.h"
#include "BSCtor.h"
#include "Parser.h"
#include "BSScope.h"
#include "BSNamed.h"
#include "BSClass.h"
#include "Lib/TObject.h"

namespace storm {

	bs::BSCtor::BSCtor(const vector<Value> &values, const vector<String> &names,
				const Scope &scope, Par<SStr> contents, const SrcPos &pos)
		: Function(Value(), Type::CTOR, values), scope(scope), contents(contents), paramNames(names), pos(pos) {

		// If we inherit from a class that does not know which thread to run on, we need a Thread as the
		// first parameter (#1, it is after the this ptr).
		RunOn runOn = values[0].type->runOn();
		needsThread = runOn.state == RunOn::runtime;
		if (needsThread) {
			Value expected = Value::thisPtr(Thread::stormType(engine()));

			if (values.size() < 2)
				throw SyntaxError(pos, L"This constructor needs to take a " + ::toS(expected)
								+ L" as the first parameter since we are threaded.");

			if (!expected.canStore(values[1]))
				throw SyntaxError(pos, L"This constructor needs to take a " + ::toS(expected) + L" as the first"
								L" parameter, not " + ::toS(values[1]));
		}

		// Rename until it is initialized!
		paramNames[0] = L" this";

		setCode(steal(CREATE(LazyCode, this, memberVoidFn(this, &BSCtor::generateCode))));
	}

	code::Variable bs::BSCtor::findThread(const GenState &s, const Actuals &params) {
		using namespace code;

		RunOn on = runOn();
		if (on.state != RunOn::runtime)
			return Function::findThread(s, params);

		// We know it is always the first parameter!
		Variable r = s.frame.createPtrVar(s.block);
		s.to << mov(r, params[1]);
		return r;
	}

	bs::CtorBody *bs::BSCtor::parse() {
		if (!contents)
			return defaultParse();

		Auto<SyntaxSet> syntax = getSyntax(scope);

		Auto<Parser> parser = CREATE(Parser, this, syntax, contents->v, contents->pos);
		nat parsed = parser->parse(L"CtorBody");
		if (parser->hasError())
			throw parser->error();

		Auto<Object> c = parser->transform(vector<Object *>(1, this));
		Auto<CtorBody> body = c.expect<CtorBody>(engine(), L"While evaluating CtorBody");

		return body.ret();
	}

	bs::CtorBody *bs::BSCtor::defaultParse() {
		Auto<CtorBody> r = CREATE(CtorBody, this, this);
		Auto<Actual> actual = CREATE(Actual, this);
		Auto<SuperCall> super = CREATE(SuperCall, this, r, actual);
		super->pos = pos;
		r->add(super);
		return r.ret();
	}

	code::Listing bs::BSCtor::generateCode() {
		Auto<CtorBody> body = parse();

		using namespace code;
		Listing l;
		CodeData data;

		l << prolog();

		// Parameters (first one is special).
		{
			Variable thisVar = l.frame.createParameter(params[0].size(), false);
			LocalVar *hidden = body->variable(L" this");
			LocalVar *normal = body->variable(L"this");
			hidden->var = VarInfo(thisVar);
			normal->var = VarInfo(thisVar);
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

	bs::LocalVar *bs::BSCtor::addParams(Par<Block> to) {
		Auto<LocalVar> thread;

		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);
			if (i == 0)
				var->constant = true;
			to->add(var);
			if (i == 1 && needsThread)
				thread = var;
		}

		return thread.ret();
	}

	bs::CtorBody::CtorBody(Par<BSCtor> ctor) : ExprBlock(ctor->scope) {
		threadParam = ctor->addParams(this);
	}

	void bs::CtorBody::add(Par<ArrayP<Expr>> exprs) {
		for (nat i = 0; i < exprs->count(); i++) {
			add(exprs->at(i));
		}
	}

	void bs::CtorBody::blockCode(const GenState &state, GenResult &to, const code::Block &block) {
		if (threadParam) {
			thread = state.frame.createPtrVar(state.block);
			state.to << mov(thread, threadParam->var.var);
		}

		Block::blockCode(state, to, block);
	}

	bs::Initializer::Initializer(Par<SStr> name, Par<Expr> expr) : name(name), expr(expr) {}

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
		rootBlock = block.borrow();

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

		initMap.insert(make_pair(name, init));
	}

	void bs::SuperCall::callParent(const GenState &s) {
		Type *parent = thisPtr.type->super();
		if (!parent)
			return;

		RunOn runOn = thisPtr.type->runOn();
		bool hiddenThread = runOn.state == RunOn::runtime;
		if (!hiddenThread && parent == TObject::stormType(engine())) {
			callTObject(s, runOn.thread);
			return;
		}

		// Find something to call.
		vector<Value> values = params->values();
		values[0].type = parent;

		if (hiddenThread)
			values.insert(values.begin() + 1, Value::thisPtr(Thread::stormType(engine())));

		Function *ctor = as<Function>(parent->find(Type::CTOR, values));
		if (!ctor)
			throw SyntaxError(pos, L"No constructor (" + join(values, L", ") + L") found in " + parent->identifier());

		vector<code::Value> actuals;
		actuals.reserve(values.size());

		if (hiddenThread) {
			actuals.push_back(params->code(0, s, ctor->params[0]));
			actuals.push_back(rootBlock->threadParam->var.var);
			for (nat i = 2; i < values.size(); i++)
				actuals.push_back(params->code(i - 1, s, ctor->params[i]));
		} else {
			for (nat i = 0; i < values.size(); i++)
				actuals.push_back(params->code(i, s, ctor->params[i]));
		}

		GenResult t;
		ctor->localCall(s, actuals, t, false);
	}

	void bs::SuperCall::callTObject(const GenState &s, Par<NamedThread> t) {
		if (params->expressions.size() != 1)
			throw SyntaxError(pos, L"Can not initialize a threaded object with parameters.");

		// Find the constructor of TObject.
		Type *parent = TObject::stormType(engine());
		vector<Value> values = valList(2, Value(parent), Value(Thread::stormType(engine())));
		Function *ctor = as<Function>(parent->find(Type::CTOR, values));
		if (!ctor)
			throw InternalError(L"The constructor of TObject: __ctor(TObject, Thread) was not found!");

		// Call the constructor.
		vector<code::Value> actuals(2);
		actuals[0] = params->code(0, s, ctor->params[0]);
		actuals[1] = t->ref();

		GenResult res;
		ctor->localCall(s, actuals, res, false);
	}

	void bs::SuperCall::code(const GenState &s, GenResult &r) {
		using namespace code;

		// Super class should be called first.
		callParent(s);

		Variable dest = thisVar->var.var;
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

	void bs::SuperCall::initVar(const GenState &s, Par<TypeVar> v) {
		InitMap::iterator i = initMap.find(v->name);
		if (i == initMap.end())
			initVarDefault(s, v);
		else
			initVar(s, v, i->second);
	}

	void bs::SuperCall::initVarDefault(const GenState &s, Par<TypeVar> v) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var.var;

		if (t.ref)
			throw SyntaxError(pos, L"Can not initialize reference " + v->name + L", not implemented yet!");

		if (t.isValue()) {
			s.to << mov(ptrA, dest);
			s.to << add(ptrA, intPtrConst(v->offset()));
			s.to << fnParam(ptrA);
			s.to << fnCall(t.defaultCtor(), Size());
		} else if (t.isBuiltIn()) {
			// Default value is already there.
		} else {
			Function *ctor = t.type->defaultCtor();
			if (!ctor)
				throw SyntaxError(pos, L"Can not initialize " + v->name + L" by default-constructing it. In "
								+ ::toS(thisPtr) + L", please initialize this member explicitly.");
			Auto<Actual> params = CREATE(Actual, this);
			Auto<CtorCall> ctorCall = CREATE(CtorCall, this, ctor, params);
			GenResult created(t, s.block);
			ctorCall->code(s, created);

			s.to << mov(ptrA, dest);
			s.to << mov(ptrRel(ptrA, v->offset()), created.location(s).var);
			s.to << code::addRef(created.location(s).var);
		}
	}

	void bs::SuperCall::initVar(const GenState &s, Par<TypeVar> v, Par<Initializer> to) {
		using namespace code;

		const Value &t = v->varType;

		if (t.ref)
			throw SyntaxError(pos, L"Can not initialize reference " + v->name + L", not implemented yet!");

		assert(to->expr || to->params);

		if (to->expr) {
			if (t.isClass() && t.canStore(to->expr->result())) {
				initVarAssign(s, v, to->expr);
			} else {
				Auto<Actual> p = CREATE(Actual, this);
				p->add(to->expr);
				initVarCtor(s, v, p);
			}
		} else {
			initVarCtor(s, v, to->params);
		}

	}

	void bs::SuperCall::initVarCtor(const GenState &s, Par<TypeVar> v, Par<Actual> to) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var.var;
		Type *toCreate = t.type;

		vector<Value> values = to->values();
		values.insert(values.begin(), Value::thisPtr(toCreate));

		Function *ctor = as<Function>(toCreate->find(Type::CTOR, values));
		if (ctor == null)
			throw SyntaxError(to->pos, L"No constructor for " + ::toS(t) + L"(" + join(values, L", ") + L").");

		if (t.isClass()) {
			// Easy way, call the constructor as normal.
			Auto<CtorCall> call = CREATE(CtorCall, this, ctor, to);
			GenResult created(t, s.block);
			call->code(s, created);
			VarInfo loc = created.location(s);
			s.to << mov(ptrA, dest);
			s.to << mov(ptrRel(ptrA, v->offset()), loc.var);
			s.to << code::addRef(loc.var);
			loc.created(s);
		} else {
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

	void bs::SuperCall::initVarAssign(const GenState &s, Par<TypeVar> v, Par<Expr> to) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var.var;
		assert(t.isClass());

		GenResult result(t, s.block);
		to->code(s, result);
		VarInfo loc = result.location(s);
		s.to << mov(ptrA, dest);
		s.to << mov(ptrRel(ptrA, v->offset()), loc.var);
		s.to << code::addRef(loc.var);
	}

}
