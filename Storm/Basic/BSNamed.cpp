#include "stdafx.h"
#include "BSNamed.h"
#include "BSBlock.h"
#include "BSAutocast.h"
#include "Exception.h"
#include "Function.h"
#include "Shared/Future.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	namespace bs {

		static void callOnThread(Par<Function> fn, Par<CodeGen> s, const Function::Actuals &actuals,
								Par<CodeResult> to, bool lookup, bool sameObject) {

			if (sameObject) {
				// Regular function call is enough.
				fn->localCall(s, actuals, to, lookup);
			} else {
				fn->autoCall(s, actuals, to);
			}
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
				s->to << fnCall(to->type().copyCtor(), retVoid());
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

	ExprResult bs::FnCall::result() {
		if (async)
			return Value(futureType(engine(), toExecute->result));
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
			if (fn->runOn().state == RunOn::any) {
				fn->asyncThreadCall(s, vars, to);
			} else {
				Variable threadVar = fn->findThread(s, vars);
				fn->asyncThreadCall(s, vars, to, threadVar);
			}
		} else {
			callFn(toExecute, s, vars, to, lookup, sameObject);
		}
	}

	void bs::FnCall::output(wostream &to) const {
		Auto<SimpleName> p = toExecute->path();
		p->last() = CREATE(SimplePart, p, p->lastName());
		to << p << params;
		if (async)
			to << L"&";
	}

	/**
	 * Execute constructor.
	 */

	bs::CtorCall::CtorCall(Par<Function> ctor, Par<Actual> params) : ctor(ctor), params(params) {
		assert(params->expressions.size() == ctor->params.size() - 1, L"Invalid number of parameters to constructor!");
		toCreate = ctor->params[0].asRef(false);
	}

	ExprResult bs::CtorCall::result() {
		return toCreate.asRef(false);
	}

	void bs::CtorCall::output(wostream &to) const {
		to << toCreate << params;
	}

	void bs::CtorCall::code(Par<CodeGen> s, Par<CodeResult> to) {
		if (toCreate.isValue() || toCreate.isBuiltIn())
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

		vector<Value> values = ctor->params;
		vector<code::Value> vars(values.size() - 1);

		for (nat i = 0; i < values.size() - 1; i++)
			vars[i] = params->code(i, s, values[i + 1]);

		VarInfo created = to->safeLocation(s, toCreate);
		allocObject(s, ctor, vars, created.var());
		created.created(s);
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

	ExprResult bs::LocalVarAccess::result() {
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
				s->to << fnCall(var->result.copyCtor(), retVoid());
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

	ExprResult bs::BareVarAccess::result() {
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
				s->to << fnCall(type.copyCtor(), retVoid());
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

	ExprResult bs::MemberVarAccess::result() {
		return var->varType.asRef();
	}

	void bs::MemberVarAccess::code(Par<CodeGen> s, Par<CodeResult> to) {
		if (!to->needed()) {
			// We still need to evaluate 'member', it may have side effects!
			Auto<CodeResult> mResult = CREATE(CodeResult, this);
			member->code(s, mResult);
			return;
		}

		if (var->owner()->typeFlags & typeValue) {
			valueCode(s, to);
		} else {
			classCode(s, to);
		}
	}

	void bs::MemberVarAccess::valueCode(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		Value mType = member->result().type();
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

		Auto<CodeResult> mResult = CREATE(CodeResult, this, member->result().type().asRef(false), s->block);
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
			s->to << fnCall(var->varType.copyCtor(), retVoid());
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

	ExprResult bs::NamedThreadAccess::result() {
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

	bs::ClassAssign::ClassAssign(Par<Expr> to, Par<Expr> value) : to(to) {
		Value r = to->result().type();
		if ((r.type->typeFlags & typeClass) != typeClass)
			throw TypeError(to->pos, L"The default assignment can not be used with other types than classes"
							L" at the moment. Please implement an assignment operator for your type.");
		if (!r.ref)
			throw TypeError(to->pos, L"Can not assign to a non-reference.");

		this->value = castTo(value, r.asRef(false));
		if (!this->value)
			throw TypeError(to->pos, L"Can not store a " + ::toS(value->result()) + L" in " + ::toS(r));
	}

	ExprResult bs::ClassAssign::result() {
		return to->result().type().asRef(false);
	}

	void bs::ClassAssign::code(Par<CodeGen> s, Par<CodeResult> to) {
		using namespace code;

		// Type to work with.
		Value t = this->to->result().type().asRef(false);

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
		static Expr *findCtor(Par<Type> t, Par<Actual> actual, const SrcPos &pos);
		static Expr *findTarget(Par<Named> n, Par<LocalVar> first, Par<Actual> actual, const SrcPos &pos, bool useLookup);
		static Expr *findTargetThis(Par<Block> block, Par<SimpleName> name,
									Par<Actual> params, const SrcPos &pos,
									Auto<Named> &candidate);
		static Expr *findTarget(Par<Block> block, Par<SimpleName> name,
								const SrcPos &pos, Par<Actual> params,
								bool useThis);
	}

	// Find a constructor.
	static bs::Expr *bs::findCtor(Par<Type> t, Par<Actual> actual, const SrcPos &pos) {
		Auto<BSNamePart> part = CREATE(BSNamePart, t->engine, Type::CTOR, actual);
		part->insert(Value::thisPtr(t));

		Auto<Function> ctor = steal(t->find(part)).as<Function>();
		if (!ctor)
			throw SyntaxError(pos, L"No constructor " + t->identifier() + ::toS(part) + L")");

		return CREATE(CtorCall, t, ctor, actual);
	}


	// Helper to create the actual type, given something found. If '!useLookup', then we will not use the lookup
	// of the function or variable (ie use vtables).
	static bs::Expr *bs::findTarget(Par<Named> n, Par<LocalVar> first, Par<Actual> actual, const SrcPos &pos, bool useLookup) {
		if (!n)
			return null;

		if (n->name == Type::CTOR)
			throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Type() instead.");
		if (n->name == Type::DTOR)
			throw SyntaxError(pos, L"Manual invocations of destructors are forbidden.");

		if (Auto<Function> f = n.as<Function>()) {
			if (first)
				actual->addFirst(steal(CREATE(LocalVarAccess, first, first)));
			return CREATE(FnCall, n, f, actual, useLookup, first ? true : false);
		}

		if (Auto<LocalVar> v = n.as<LocalVar>()) {
			assert(!first);
			return CREATE(LocalVarAccess, n, v);
		}

		if (Auto<TypeVar> v = n.as<TypeVar>()) {
			if (first)
				return CREATE(MemberVarAccess, n, steal(CREATE(LocalVarAccess, first, first)), v);
			else
				return CREATE(MemberVarAccess, n, actual->expressions.front(), v);
		}

		if (Auto<NamedThread> v = n.as<NamedThread>()) {
			assert(!first);
			return CREATE(NamedThreadAccess, n, v);
		}

		return null;
	}

	static bool isSuperName(Par<SimpleName> name) {
		if (name->count() != 2)
			return false;

		const Auto<SimplePart> &p = name->at(0);
		if (p->name != L"super")
			return false;
		return p->empty();
	}

	// Find a target assuming we should use the this-pointer.
	static bs::Expr *bs::findTargetThis(Par<Block> block, Par<SimpleName> name,
										Par<Actual> params, const SrcPos &pos,
										Auto<Named> &candidate) {
		const Scope &scope = block->scope;

		Auto<SimplePart> thisPart = CREATE(SimplePart, block, L"this");
		Auto<LocalVar> thisVar = block->variable(thisPart);
		if (!thisVar)
			return null;

		Auto<BSNamePart> lastPart = CREATE(BSNamePart, name, name->lastName(), params);
		lastPart->insert(thisVar->result);
		bool useLookup = true;

		if (isSuperName(name)) {
			Auto<SimpleName> part = name->from(1);
			// It is something in the super type!
			Type *super = thisVar->result.type->super();
			if (!super)
				throw SyntaxError(pos, L"No super type for " + ::toS(thisVar->result) + L", can not use 'super' here.");

			part->last() = lastPart;
			candidate = storm::find(super, part);
			useLookup = false;
		} else {
			// May be anything.
			name->last() = lastPart;
			candidate = scope.find(name);
			useLookup = true;
		}

		Expr *e = findTarget(candidate, thisVar, params, pos, useLookup);
		if (e)
			return e;

		return null;
	}

	// Find whatever is meant by the 'name' in this context. Return suitable expression. If
	// 'useThis' is true, a 'this' pointer may be inserted as the first parameter.
	static bs::Expr *bs::findTarget(Par<Block> block, Par<SimpleName> name,
									const SrcPos &pos, Par<Actual> params,
									bool useThis) {
		const Scope &scope = block->scope;

		// Type ctors and local variables have priority.
		{
			Auto<Named> n = scope.find(name);
			if (Auto<Type> t = n.as<Type>())
				return findCtor(t, params, pos);
			else if (as<LocalVar>(n.borrow()) != null && params->empty())
				return findTarget(n, null, params, pos, false);
		}

		// If we have a this-pointer, try to use it!
		Auto<Named> candidate;
		if (useThis)
			if (Expr *e = findTargetThis(block, name, params, pos, candidate))
				return e;

		// Try without the this pointer first.
		Auto<BSNamePart> last = CREATE(BSNamePart, name, name->lastName(), params);
		name->last() = last;
		Auto<Named> n = scope.find(name);

		if (Expr *e = findTarget(n, null, params, pos, true))
			return e;

		if (!n && !candidate)
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L".");

		if (!n)
			n = candidate;

		throw TypeError(pos, ::toS(n) + L" is a " + n->myType->identifier() +
						L". Only functions, variables and constructors are supported.");
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Actual> params) {
		Auto<SimpleName> n = CREATE(SimpleName, name, name->v);
		return findTarget(block, n, name->pos, params, true);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SrcName> name, Par<Actual> params) {
		return namedExpr(block, name->pos, name, params);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, SrcPos pos, Par<Name> name, Par<Actual> params) {
		Auto<SimpleName> simple = name->simplify(block->scope);
		if (!simple)
			throw SyntaxError(pos, L"Could not resolve parameters in " + ::toS(name));

		return findTarget(block, simple, pos, params, true);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first, Par<Actual> params) {
		params->addFirst(first);
		Auto<SimpleName> n = CREATE(SimpleName, name, name->v);
		return findTarget(block, n, name->pos, params, false);
	}

	bs::Expr *bs::namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first) {
		Auto<Actual> params = CREATE(Actual, block);
		params->add(first);
		Auto<SimpleName> n = CREATE(SimpleName, name, name->v);
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
