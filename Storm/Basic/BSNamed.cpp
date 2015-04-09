#include "stdafx.h"
#include "BSNamed.h"
#include "BSBlock.h"
#include "Exception.h"
#include "Function.h"
#include "Lib/Future.h"

namespace storm {

	namespace bs {

		static void callOnThread(Par<Function> fn, Par<CodeGen> s, const Function::Actuals &actuals,
								Par<CodeResult> to, bool lookup, bool sameObject) {
			RunOn target = fn->runOn();
			if (sameObject || s->runOn.canRun(target)) {
				// Regular function call is  enough.
				fn->localCall(s, actuals, to, lookup);
				return;
			}

			assert(target.state != RunOn::any);

			code::Variable threadVar = fn->findThread(s, actuals);
			fn->threadCall(s, actuals, to, threadVar);
		}

		// Call the function and handle the result correctly.
		static void callFn(Par<Function> call, Par<CodeGen> s, const Function::Actuals &values,
						Par<CodeResult> to, bool lookup, bool sameObject) {
			using namespace code;

			if (to->type().ref == call->result.ref) {
				callOnThread(call, s, values, to, lookup, sameObject);
				return;
			}

			// We need to do stuff!
			Auto<CodeResult> t = CREATE(CodeResult, call, call->result, s->block);
			callOnThread(call, s, values, t, lookup, sameObject);

			if (!to->needed())
				return;

			VarInfo r = to->location(s);
			if (to->type().ref) {
				// Dangerous...
				s->to << lea(r.var(), ptrRel(t->location(s).var()));
			} else if (to->type().isValue()) {
				// Need to copy...
				s->to << lea(ptrA, ptrRel(r.var()));
				s->to << fnParam(ptrA);
				s->to << fnParam(t->location(s).var());
				s->to << fnCall(to->type().copyCtor(), Size());
			} else {
				// Regular machine operations suffice!
				s->to << mov(ptrA, t->location(s).var());
				s->to << mov(r.var(), xRel(to->type().size(), ptrA));
				if (to->type().refcounted())
					s->to << addRef(r.var());
			}
			r.created(s);
		}

	}


	/**
	 * Function call.
	 */

	bs::FnCall::FnCall(Par<Function> toExecute, Par<Actual> params, Bool lookup, Bool sameObject)
		: toExecute(toExecute), params(params), lookup(lookup), sameObject(sameObject), async(false) {}

	bs::FnCall::FnCall(Par<Function> toExecute, Par<Actual> params)
		: toExecute(toExecute), params(params), lookup(true), sameObject(false), async(false) {}

	void bs::FnCall::makeAsync() {
		async = true;
	}

	Value bs::FnCall::result() {
		if (async)
			return futureType(engine(), toExecute->result);
		else
			return toExecute->result;
	}

	void bs::FnCall::code(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;
		const Value &r = toExecute->result;

		// Note that toExecute->params may not be equal to params->values()
		// since some actual parameters may report that they can return a
		// reference to a value. However, we know that toExecute->params.canStore(params->values())
		// so it is OK to take toExecute->params directly.
		const vector<Value> &values = toExecute->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		for (nat i = 0; i < values.size(); i++)
			vars[i] = params->code(i, s, values[i]);

		// Call!
		if (async) {
			// We will here more or less force to run on another uthread, even though we would
			// not strictly need to in all cases.
			Function *fn = toExecute.borrow();
			if (fn->runOn().state == RunOn::any)
				throw SyntaxError(pos, L"The function " + fn->identifier() + L" does not specify a thread"
								L"to execute on, and can therefore not be spawned using the async syntax.");

			Variable threadVar = fn->findThread(s, vars);
			fn->asyncThreadCall(s, vars, to, threadVar);
		} else {
			callFn(toExecute, s, vars, to, lookup, sameObject);
		}
	}

	void bs::FnCall::output(wostream &to) const {
		Auto<Name> p = toExecute->path();
		p = p->withParams(vector<Value>());
		to << p << params;
		if (async)
			to << L"&";
	}

	/**
	 * Execute constructor.
	 */

	bs::CtorCall::CtorCall(Par<Function> ctor, Par<Actual> params) : ctor(ctor), params(params) {
		toCreate = ctor->params[0].asRef(false);
	}

	Value bs::CtorCall::result() {
		return toCreate.asRef(false);
	}

	void bs::CtorCall::output(wostream &to) const {
		to << toCreate << params;
	}

	void bs::CtorCall::code(Par<CodeGen> s, Par<CodeResult> to) {
		if (toCreate.isValue())
			createValue(s, to);
		else
			createClass(s, to);
	}

	void bs::CtorCall::createValue(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;
		Engine &e = Object::engine();

		// Call the constructor with the address of our variable.
		vector<Value> values = ctor->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		for (nat i = 1; i < values.size(); i++)
			vars[i] = params->code(i - 1, s, values[i]);

		// This needs to be last, otherwise other generated code may overwrite it!
		VarInfo thisVar;
		if (to->type().ref) {
			thisVar = to->location(s);
			vars[0] = thisVar.var();
		} else {
			thisVar = to->safeLocation(s, toCreate.asRef(false));
			vars[0] = code::Value(ptrA);
			s->to << lea(vars[0], thisVar.var());
		}

		// Call it!
		Auto<CodeResult> voidTo = CREATE(CodeResult, this, Value(), s->block);
		ctor->localCall(s, vars, voidTo, false);
		thisVar.created(s);
	}

	void bs::CtorCall::createClass(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		Engine &e = Object::engine();

		code::Block subBlock = s->frame.createChild(s->frame.last(s->block.v));
		s->to << begin(subBlock);
		Auto<CodeGen> subState = s->child(subBlock);

		// Only free this one automatically on an exception. If there is no exception,
		// the memory will be owned by the object itself.
		Variable rawMemory = s->frame.createPtrVar(subBlock, e.fnRefs.freeRef, freeOnException);

		// Allocate memory into our temporary variable.
		s->to << fnParam(toCreate.type->typeRef);
		s->to << fnCall(e.fnRefs.allocRef, Size::sPtr);
		s->to << mov(rawMemory, ptrA);

		// Call the constructor:
		vector<Value> values = ctor->params;
		vector<code::Value> vars(values.size());

		// Load parameters.
		vars[0] = rawMemory;

		for (nat i = 1; i < values.size(); i++)
			vars[i] = params->code(i - 1, subState, values[i]);

		// Call!
		Auto<CodeResult> voidTo = CREATE(CodeResult, this, Value(), subBlock);
		callFn(ctor, subState, vars, voidTo, false, false);

		// Store our result.
		VarInfo created = to->location(subState);
		s->to << mov(created.var(), rawMemory);
		created.created(s);

		s->to << end(subBlock);
	}


	bs::CtorCall *bs::defaultCtor(const SrcPos &pos, Par<Type> t) {
		Function *f = t->defaultCtor();
		if (!f)
			throw SyntaxError(pos, L"No default constructor for " + t->identifier());

		Auto<Actual> actual = CREATE(Actual, t);
		return CREATE(CtorCall, t, f, actual);
	}

	bs::CtorCall *bs::copyCtor(const SrcPos &pos, Par<Type> t, Par<Expr> src) {
		Function *f = t->copyCtor();
		if (!f)
			throw SyntaxError(pos, L"No copy-constructor for " + t->identifier());

		Auto<Actual> actual = CREATE(Actual, t);
		actual->add(src);
		return CREATE(CtorCall, t, f, actual);
	}


	/**
	 * Local variable.
	 */
	bs::LocalVarAccess::LocalVarAccess(Par<LocalVar> var) : var(var) {}

	Value bs::LocalVarAccess::result() {
		if (var->constant)
			return var->result;
		else
			return var->result.asRef();
	}

	void bs::LocalVarAccess::code(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		if (!to->needed())
			return;

		if (to->type().ref && !var->result.ref) {
			VarInfo v = to->location(s);
			s->to << lea(v.var(), var->var.var());
			v.created(s);
		} else if (!to->suggest(s, var->var.var())) {

			VarInfo v = to->location(s);
			if (var->result.isValue()) {
				s->to << lea(ptrA, var->var.var());
				s->to << lea(ptrC, v.var());
				s->to << fnParam(ptrC);
				s->to << fnParam(ptrA);
				s->to << fnCall(var->result.copyCtor(), Size());
			} else {
				s->to << mov(v.var(), var->var.var());
				if (var->result.refcounted())
					s->to << code::addRef(v.var());
			}
			v.created(s);
		}
	}

	void bs::LocalVarAccess::output(wostream &to) const {
		to << var->name;
	}

	/**
	 * Bare local variable.
	 */
	bs::BareVarAccess::BareVarAccess(Value type, wrap::Variable var) : type(type), var(var) {}

	Value bs::BareVarAccess::result() {
		return type;
	}

	void bs::BareVarAccess::code(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		if (!to->needed())
			return;

		if (to->type().ref && !type.ref) {
			VarInfo v = to->location(s);
			s->to << lea(v.var(), var.v);
			v.created(s);
		} else if (!to->suggest(s, var.v)) {
			VarInfo v = to->location(s);
			if (type.isValue()) {
				s->to << lea(ptrA, var.v);
				s->to << lea(ptrC, v.var());
				s->to << fnParam(ptrC);
				s->to << fnParam(ptrA);
				s->to << fnCall(type.copyCtor(), Size());
			} else {
				s->to << mov(v.var(), var.v);
				if (type.refcounted())
					s->to << code::addRef(v.var());
			}
			v.created(s);
		}
	}

	void bs::BareVarAccess::output(wostream &to) const {
		to << type << L"<bare>" << code::Value(var.v);
	}

	/**
	 * Member variable.
	 */
	bs::MemberVarAccess::MemberVarAccess(Par<Expr> member, Par<TypeVar> var) : member(member), var(var) {}

	Value bs::MemberVarAccess::result() {
		return var->varType.asRef();
	}

	void bs::MemberVarAccess::code(Par<CodeGen> s, Par<CodeResult> to) {
		if (!to->needed()) {
			// We still need to evaluate 'member', it may have side effects!
			Auto<CodeResult> mResult = CREATE(CodeResult, this);
			member->code(s, mResult);
			return;
		}

		if (var->owner()->flags & typeValue) {
			valueCode(s, to);
		} else {
			classCode(s, to);
		}
	}

	void bs::MemberVarAccess::valueCode(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		Value mType = member->result();
		code::Variable memberPtr;

		{
			Auto<CodeResult> mResult = CREATE(CodeResult, this, mType, s->block);
			member->code(s, mResult);
			memberPtr = mResult->location(s).var();
		}

		// If it was a reference, we can use it right away!
		if (mType.ref) {
			s->to << mov(ptrA, memberPtr);
		} else {
			s->to << lea(ptrA, memberPtr);
		}

		extractCode(s, to);
	}

	void bs::MemberVarAccess::classCode(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		Auto<CodeResult> mResult = CREATE(CodeResult, this, member->result().asRef(false), s->block);
		member->code(s, mResult);
		s->to << mov(ptrA, mResult->location(s).var());

		extractCode(s, to);
	}

	void bs::MemberVarAccess::extractCode(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		VarInfo result = to->location(s);
		if (to->type().ref) {
			s->to << add(ptrA, intPtrConst(var->offset()));
			s->to << mov(result.var(), ptrA);
		} else if (var->varType.isValue()) {
			s->to << add(ptrA, intPtrConst(var->offset()));
			s->to << lea(ptrC, result.var());
			s->to << fnParam(ptrC);
			s->to << fnParam(ptrA);
			s->to << fnCall(var->varType.copyCtor(), Size());
		} else {
			s->to << mov(result.var(), xRel(result.var().size(), ptrA, var->offset()));
			if (var->varType.refcounted())
				s->to << code::addRef(result.var());
		}
		result.created(s);
	}

	void bs::MemberVarAccess::output(wostream &to) const {
		to << member << L"." << var->name;
	}


	/**
	 * Named thread.
	 */

	bs::NamedThreadAccess::NamedThreadAccess(Par<NamedThread> thread) : thread(thread) {}

	Value bs::NamedThreadAccess::result() {
		return Value(Thread::stormType(engine()));
	}

	void bs::NamedThreadAccess::code(Par<CodeGen> s, Par<CodeResult> to) {
		if (to->needed()) {
			VarInfo z = to->location(s);
			s->to << code::mov(z.var(), thread->ref());
			s->to << code::addRef(thread->ref());
			z.created(s);
		}
	}


	/**
	 * Assignment.
	 */

	bs::ClassAssign::ClassAssign(Par<Expr> to, Par<Expr> value) : to(to), value(value) {
		Value r = to->result();
		if ((r.type->flags & typeClass) != typeClass)
			throw TypeError(to->pos, L"The default assignment can not be used with other types than classes"
							L" at the moment. Please implement an assignment operator for your type.");
		if (!r.ref)
			throw TypeError(to->pos, L"Can not assign to a non-reference.");
	}

	Value bs::ClassAssign::result() {
		return to->result().asRef(false);
	}

	void bs::ClassAssign::code(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		// Type to work with.
		Value t = this->to->result().asRef(false);

		// Target variable.
		Auto<CodeResult> lhs = CREATE(CodeResult, this, t.asRef(true), s->block);
		this->to->code(s, lhs);
		code::Value targetAddr = lhs->location(s).var();

		// Compute RHS...
		Auto<CodeResult> rhs = CREATE(CodeResult, this, t, s->block);
		value->code(s, rhs);

		// Erase the previous value.
		if (t.refcounted()) {
			s->to << code::mov(ptrA, targetAddr);
			s->to << code::releaseRef(ptrRel(ptrA));
			s->to << code::addRef(rhs->location(s).var());
		}
		s->to << code::mov(ptrRel(ptrA), rhs->location(s).var());

		// Do we need to return some value?
		if (!to->needed())
			return;

		if (to->type().ref) {
			if (!to->suggest(s, targetAddr)) {
				VarInfo z = to->location(s);
				s->to << mov(z.var(), targetAddr);
				z.created(s);
			}
		} else {
			VarInfo z = to->location(s);
			s->to << mov(ptrA, targetAddr);
			s->to << mov(z.var(), ptrRel(ptrA));
			s->to << code::addRef(z.var());
			z.created(s);
		}
	}


	/**
	 * Look up a proper action from a name and a set of parameters.
	 */
	namespace bs {
		static Expr *findCtor(Type *t, Par<Actual> actual, const SrcPos &pos);
		static Expr *findTarget(Named *n, Par<LocalVar> first, Par<Actual> actual, const SrcPos &pos, bool useLookup);
		static Expr *findTargetThis(Par<Block> block, Par<Name> name,
									Par<Actual> params, const SrcPos &pos,
									Named *&candidate);
		static Expr *findTarget(Par<Block> block, Par<Name> name,
								const SrcPos &pos, Par<Actual> params,
								bool useThis);
	}

	// Find a constructor.
	static bs::Expr *bs::findCtor(Type *t, Par<Actual> actual, const SrcPos &pos) {
		vector<Value> params = actual->values();
		params.insert(params.begin(), Value::thisPtr(t));

		Function *ctor = as<Function>(t->find(Type::CTOR, params));
		if (!ctor)
			throw SyntaxError(pos, L"No constructor " + t->identifier() + L"(" + join(params, L", ") + L")");

		return CREATE(CtorCall, t, ctor, actual);
	}


	// Helper to create the actual type, given something found. If '!useLookup', then we will not use the lookup
	// of the function or variable (ie use vtables).
	static bs::Expr *bs::findTarget(Named *n, Par<LocalVar> first, Par<Actual> actual, const SrcPos &pos, bool useLookup) {
		if (!n)
			return null;

		if (n->name == Type::CTOR)
			throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Type() instead.");
		if (n->name == Type::DTOR)
			throw SyntaxError(pos, L"Manual invocations of destructors are forbidden.");

		if (Function *f = as<Function>(n)) {
			if (first)
				actual->addFirst(steal(CREATE(LocalVarAccess, first, first)));
			return CREATE(FnCall, n, f, actual, useLookup, first ? true : false);
		}

		if (LocalVar *v = as<LocalVar>(n)) {
			assert(!first);
			return CREATE(LocalVarAccess, n, v);
		}

		if (TypeVar *v = as<TypeVar>(n)) {
			if (first)
				return CREATE(MemberVarAccess, n, steal(CREATE(LocalVarAccess, first, first)), v);
			else
				return CREATE(MemberVarAccess, n, actual->expressions.front(), v);
		}

		if (NamedThread *v = as<NamedThread>(n)) {
			assert(!first);
			return CREATE(NamedThreadAccess, n, v);
		}

		return null;
	}

	static bool isSuperName(Par<Name> name) {
		if (name->size() <= 1)
			return false;

		NamePart *p = name->at(0);
		if (p->name != L"super")
			return false;
		return p->params.size() == 0;
	}

	// Find a target assuming we should use the this-pointer.
	static bs::Expr *bs::findTargetThis(Par<Block> block, Par<Name> name,
										Par<Actual> params, const SrcPos &pos,
										Named *&candidate) {
		const Scope &scope = block->scope;

		LocalVar *thisVar = block->variable(L"this");
		if (!thisVar)
			return null;

		vector<Value> vals = params->values();
		vals.insert(vals.begin(), thisVar->result);
		bool useLookup = true;

		if (isSuperName(name)) {
			Auto<Name> part = name->from(1);
			// It is something in the super type!
			Type *super = thisVar->result.type->super();
			if (!super)
				throw SyntaxError(pos, L"No super type for " + ::toS(thisVar->result) + L", can not use 'super' here.");

			candidate = storm::find(super, steal(part->withParams(vals)));
			useLookup = false;
		} else {
			// May be anything.
			candidate = scope.find(steal(name->withParams(vals)));
			useLookup = true;
		}

		Named *n = candidate;
		Expr *e = findTarget(n, thisVar, params, pos, useLookup);
		if (e)
			return e;

		return null;
	}

	// Find whatever is meant by the 'name' in this context. Return suitable expression. If
	// 'useThis' is true, a 'this' pointer may be inserted as the first parameter.
	static bs::Expr *bs::findTarget(Par<Block> block, Par<Name> name,
									const SrcPos &pos, Par<Actual> params,
									bool useThis) {
		const Scope &scope = block->scope;

		if (Type *t = as<Type>(scope.find(name)))
			return findCtor(t, params, pos);

		// Try without the this pointer first.
		Named *n = scope.find(steal(name->withParams(params->values())));

		if (Expr *e = findTarget(n, null, params, pos, true))
			return e;

		// If we have a this-pointer, try to use it!
		Named *candidate = null;
		if (useThis)
			if (Expr *e = findTargetThis(block, name, params, pos, candidate))
				return e;


		if (!n && !candidate)
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L"("
							+ join(params->values(), L", ") + L").");

		if (!n)
			n = candidate;

		throw TypeError(pos, ::toS(n) + L" is a " + n->myType->identifier() +
						L". Only functions, variables and constructors are supported.");
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Actual> params) {
		Auto<Name> n = parseSimpleName(name->engine(), name->v->v);
		return findTarget(block, n, name->pos, params, true);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<TypeName> name, Par<Actual> params) {
		if (name->count() == 1 && params->expressions.size() == 0) {
			TypePart *part = name->parts[0].borrow();
			if (part->params.empty()) {
				const String &n = part->name->v;
				if (n == L"true")
					return CREATE(Constant, block, true);
				else if (n == L"false")
					return CREATE(Constant, block, false);
			}
		}
		Auto<Name> n = name->toName(block->scope);
		return findTarget(block, n, name->pos, params, true);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first, Par<Actual> params) {
		params->addFirst(first);
		Auto<Name> n = parseSimpleName(name->engine(), name->v->v);
		return findTarget(block, n, name->pos, params, false);
	}

	bs::Expr *STORM_FN bs::spawnExpr(Par<Expr> expr) {
		FnCall *fnCall = as<FnCall>(expr.borrow());
		if (!fnCall)
			throw SyntaxError(expr->pos, L"The spawn-syntax is not applicable to anything but functions"
							L" at the moment. This is a " + expr->myType->identifier());

		fnCall->makeAsync();

		return expr.ret();
	}

}
