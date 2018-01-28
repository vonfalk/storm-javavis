#include "stdafx.h"
#include "Named.h"
#include "Exception.h"
#include "Cast.h"
#include "Lookup.h"
#include "Actuals.h"
#include "Block.h"
#include "Compiler/Engine.h"
#include "Compiler/Lib/Future.h"

namespace storm {
	namespace bs {

		static void callOnThread(Function *fn, CodeGen *s, Array<code::Operand> *actuals,
								CodeResult *to, bool lookup, bool sameObject) {

			if (sameObject) {
				// Regular function call is enough.
				fn->localCall(s, actuals, to, lookup);
			} else {
				fn->autoCall(s, actuals, to);
			}
		}

		// Call the function and handle the result correctly.
		static void callFn(Function *call, CodeGen *s, Array<code::Operand> *values,
						CodeResult *to, bool lookup, bool sameObject) {
			using namespace code;

			Engine &e = call->engine();
			if (!to->needed() || to->type().ref == call->result.ref) {
				callOnThread(call, s, values, to, lookup, sameObject);
				return;
			}

			// We need to do stuff!
			CodeResult *t = new (call) CodeResult(call->result, s->block);
			callOnThread(call, s, values, t, lookup, sameObject);

			if (!to->needed())
				return;

			VarInfo r = to->location(s);
			if (to->type().ref) {
				// Dangerous...
				*s->l << lea(r.v, ptrRel(t->location(s).v, Offset()));
			} else if (to->type().isValue()) {
				// Need to copy...
				*s->l << lea(ptrA, ptrRel(r.v, Offset()));
				*s->l << fnParam(e.ptrDesc(), ptrA);
				*s->l << fnParam(e.ptrDesc(), t->location(s).v);
				*s->l << fnCall(to->type().copyCtor(), true);
			} else {
				// Regular machine operations suffice!
				*s->l << mov(ptrA, t->location(s).v);
				*s->l << mov(r.v, xRel(to->type().size(), ptrA, Offset()));
			}
			r.created(s);
		}


		/**
		 * Function call.
		 */

		FnCall::FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params, Bool lookup, Bool sameObject)
			: Expr(pos), toExecute(toExecute), params(params), scope(scope), lookup(lookup), sameObject(sameObject), async(false) {

			if (params->expressions->count() != toExecute->params->count())
				throw SyntaxError(pos, L"The parameter count does not match!");
		}

		FnCall::FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params)
			: Expr(pos), toExecute(toExecute), params(params), scope(scope), lookup(true), sameObject(false), async(false) {

			if (params->expressions->count() != toExecute->params->count())
				throw SyntaxError(pos, L"The parameter count does not match!");
		}

		void FnCall::makeAsync() {
			async = true;
		}

		ExprResult FnCall::result() {
			if (async)
				return wrapFuture(engine(), toExecute->result);
			else
				return toExecute->result;
		}

		void FnCall::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			// Note that toExecute->params may not be equal to params->values()
			// since some actual parameters may report that they can return a
			// reference to a value. However, we know that toExecute->params.canStore(params->values())
			// so it is OK to take toExecute->params directly.
			Array<Value> *values = toExecute->params;
			Array<code::Operand> *vars = new (this) Array<code::Operand>(values->count());

			// Load parameters.
			for (nat i = 0; i < values->count(); i++)
				vars->at(i) = params->code(i, s, values->at(i), scope);

			// Call!
			if (async) {
				// We will here more or less force to run on another uthread, even though we would
				// not strictly need to in all cases.
				Function *fn = toExecute;
				if (fn->runOn().state == RunOn::any) {
					fn->asyncThreadCall(s, vars, to);
				} else {
					code::Var threadVar = fn->findThread(s, vars);
					fn->asyncThreadCall(s, vars, to, threadVar);
				}
			} else {
				callFn(toExecute, s, vars, to, lookup, sameObject);
			}
		}

		void FnCall::toS(StrBuf *to) const {
			SimpleName *p = toExecute->path();
			p->last() = new (p) SimplePart(p->last()->name);
			*to << p << params;
			if (async)
				*to << L"&";
		}

		/**
		 * Execute constructor.
		 */

		CtorCall::CtorCall(SrcPos pos, Scope scope, Function *ctor, Actuals *params)
			: Expr(pos), ctor(ctor), params(params), scope(scope) {

			assert(params->expressions->count() == ctor->params->count() - 1, L"Invalid number of parameters to constructor!");
			toCreate = ctor->params->at(0).asRef(false);
		}

		ExprResult CtorCall::result() {
			return toCreate.asRef(false);
		}

		void CtorCall::toS(StrBuf *to) const {
			*to << toCreate << params;
		}

		void CtorCall::code(CodeGen *s, CodeResult *to) {
			if (toCreate.isValue() || toCreate.isBuiltIn())
				createValue(s, to);
			else
				createClass(s, to);
		}

		void CtorCall::createValue(CodeGen *s, CodeResult *to) {
			using namespace code;
			Engine &e = engine();

			// Call the constructor with the address of our variable.
			Array<Value> *values = ctor->params;
			Array<code::Operand> *vars = new (e) Array<code::Operand>(values->count());

			// Load parameters.
			for (nat i = 1; i < values->count(); i++)
				vars->at(i) = params->code(i - 1, s, values->at(i), scope);

			// This needs to be last, otherwise other generated code may overwrite it!
			VarInfo thisVar;
			if (to->type().ref) {
				thisVar = to->location(s);
				vars->at(0) = thisVar.v;
			} else {
				thisVar = to->safeLocation(s, toCreate.asRef(false));
				vars->at(0) = code::Operand(ptrA);
				*s->l << lea(vars->at(0), thisVar.v);
			}

			// Call it!
			CodeResult *voidTo = new (this) CodeResult(Value(), s->block);
			ctor->localCall(s, vars, voidTo, false);
			thisVar.created(s);
		}

		void CtorCall::createClass(CodeGen *s, CodeResult *to) {
			using namespace code;
			Engine &e = engine();

			Array<Value> *values = ctor->params;
			Array<code::Operand> *vars = new (e) Array<code::Operand>(values->count() - 1);

			for (nat i = 0; i < values->count() - 1; i++)
				vars->at(i) = params->code(i, s, values->at(i + 1), scope);

			VarInfo created = to->safeLocation(s, toCreate);
			allocObject(s, ctor, vars, created.v);
			created.created(s);
		}

		CtorCall *defaultCtor(const SrcPos &pos, Scope scope, Type *t) {
			Function *f = t->defaultCtor();
			if (!f)
				throw SyntaxError(pos, L"No default constructor for " + ::toS(t->identifier()));

			Actuals *actual = new (t) Actuals();
			return new (t) CtorCall(pos, scope, f, actual);
		}

		CtorCall *copyCtor(const SrcPos &pos, Scope scope, Type *t, Expr *src) {
			Function *f = t->copyCtor();
			if (!f)
				throw SyntaxError(pos, L"No copy-constructor for " + ::toS(t->identifier()));

			Actuals *actual = new (t) Actuals();
			actual->add(src);
			return new (t) CtorCall(pos, scope, f, actual);
		}


		/**
		 * Local variable.
		 */
		LocalVarAccess::LocalVarAccess(SrcPos pos, LocalVar *var) : Expr(pos), var(var) {}

		ExprResult LocalVarAccess::result() {
			if (var->constant)
				return var->result;
			else
				return var->result.asRef();
		}

		void LocalVarAccess::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			if (!to->needed())
				return;

			if (to->type().ref && !var->result.ref) {
				VarInfo v = to->location(s);
				*s->l << lea(v.v, var->var.v);
				v.created(s);
			} else if (!to->suggest(s, var->var.v)) {

				VarInfo v = to->location(s);
				if (var->result.isValue()) {
					*s->l << lea(ptrA, var->var.v);
					*s->l << lea(ptrC, v.v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), ptrA);
					*s->l << fnCall(var->result.copyCtor(), true);
				} else {
					*s->l << mov(v.v, var->var.v);
				}
				v.created(s);
			}
		}

		void LocalVarAccess::toS(StrBuf *to) const {
			*to << var->name;
		}

		/**
		 * Bare local variable.
		 */
		BareVarAccess::BareVarAccess(SrcPos pos, Value type, code::Var var) : Expr(pos), type(type), var(var) {}

		ExprResult BareVarAccess::result() {
			return type;
		}

		void BareVarAccess::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			if (!to->needed())
				return;

			if (to->type().ref && !type.ref) {
				VarInfo v = to->location(s);
				*s->l << lea(v.v, var);
				v.created(s);
			} else if (!to->suggest(s, var)) {
				VarInfo v = to->location(s);
				if (type.isValue()) {
					*s->l << lea(ptrA, var);
					*s->l << lea(ptrC, v.v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), ptrA);
					*s->l << fnCall(type.copyCtor(), true);
				} else {
					*s->l << mov(v.v, var);
				}
				v.created(s);
			}
		}

		void BareVarAccess::toS(StrBuf *to) const {
			*to << type << L"<bare>" << code::Operand(var);
		}

		/**
		 * Member variable.
		 */
		MemberVarAccess::MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var)
			: Expr(pos), member(member), var(var) {}

		ExprResult MemberVarAccess::result() {
			return var->type.asRef();
		}

		void MemberVarAccess::code(CodeGen *s, CodeResult *to) {
			if (!to->needed()) {
				// We still need to evaluate 'member', it may have side effects!
				CodeResult *mResult = CREATE(CodeResult, this);
				member->code(s, mResult);
				return;
			}

			if (var->owner()->typeFlags & typeValue) {
				valueCode(s, to);
			} else {
				classCode(s, to);
			}
		}

		void MemberVarAccess::valueCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			Value mType = member->result().type();
			code::Var memberPtr;

			{
				CodeResult *mResult = new (this) CodeResult(mType, s->block);
				member->code(s, mResult);
				memberPtr = mResult->location(s).v;
			}

			// If it was a reference, we can use it right away!
			if (mType.ref) {
				*s->l << mov(ptrA, memberPtr);
			} else {
				*s->l << lea(ptrA, memberPtr);
			}

			extractCode(s, to);
		}

		void MemberVarAccess::classCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			CodeResult *mResult = new (this) CodeResult(member->result().type().asRef(false), s->block);
			member->code(s, mResult);
			*s->l << mov(ptrA, mResult->location(s).v);

			extractCode(s, to);
		}

		void MemberVarAccess::extractCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			VarInfo result = to->location(s);
			if (to->type().ref) {
				*s->l << add(ptrA, ptrConst(var->offset()));
				*s->l << mov(result.v, ptrA);
			} else if (var->type.isValue()) {
				*s->l << add(ptrA, ptrConst(var->offset()));
				*s->l << lea(ptrC, result.v);
				*s->l << fnParam(engine().ptrDesc(), ptrC);
				*s->l << fnParam(engine().ptrDesc(), ptrA);
				*s->l << fnCall(var->type.copyCtor(), true);
			} else {
				*s->l << mov(result.v, xRel(result.v.size(), ptrA, var->offset()));
			}
			result.created(s);
		}

		void MemberVarAccess::toS(StrBuf *to) const {
			*to << member << L"." << var->name;
		}


		/**
		 * Named thread.
		 */

		NamedThreadAccess::NamedThreadAccess(SrcPos pos, NamedThread *thread)
			: Expr(pos), thread(thread) {}

		ExprResult NamedThreadAccess::result() {
			return Value(Thread::stormType(engine()));
		}

		void NamedThreadAccess::code(CodeGen *s, CodeResult *to) {
			if (to->needed()) {
				VarInfo z = to->location(s);
				*s->l << code::mov(z.v, thread->ref());
				z.created(s);
			}
		}

		void NamedThreadAccess::toS(StrBuf *to) const {
			*to << thread->name;
		}


		/**
		 * Assignment.
		 */

		ClassAssign::ClassAssign(Expr *to, Expr *value, Scope scope) : Expr(to->pos), to(to) {
			Value r = to->result().type();
			if ((r.type->typeFlags & typeClass) != typeClass)
				throw TypeError(to->pos, L"The default assignment can not be used with other types than classes"
								L" at the moment. Please implement an assignment operator for your type.");
			if (!r.ref)
				throw TypeError(to->pos, L"Can not assign to a non-reference.");

			this->value = castTo(value, r.asRef(false), scope);
			if (!this->value)
				throw TypeError(to->pos, L"Can not store a " + ::toS(value->result()) + L" in " + ::toS(r));
		}

		ExprResult ClassAssign::result() {
			return to->result().type().asRef(false);
		}

		void ClassAssign::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			// Type to work with.
			Value t = this->to->result().type().asRef(false);

			// Target variable.
			CodeResult *lhs = new (this) CodeResult(t.asRef(true), s->block);
			this->to->code(s, lhs);
			code::Operand targetAddr = lhs->location(s).v;

			// Compute RHS...
			CodeResult *rhs = new (this) CodeResult(t, s->block);
			value->code(s, rhs);

			// Copy.
			*s->l << code::mov(ptrA, targetAddr);
			*s->l << code::mov(ptrRel(ptrA, Offset()), rhs->location(s).v);

			// Do we need to return some value?
			if (!to->needed())
				return;

			if (to->type().ref) {
				if (!to->suggest(s, targetAddr)) {
					VarInfo z = to->location(s);
					*s->l << mov(z.v, targetAddr);
					z.created(s);
				}
			} else {
				VarInfo z = to->location(s);
				*s->l << mov(ptrA, targetAddr);
				*s->l << mov(z.v, ptrRel(ptrA, Offset()));
				z.created(s);
			}
		}

		void ClassAssign::toS(StrBuf *to) const {
			*to << to << L" = " << value;
		}


		/**
		 * Comparison.
		 */

		ClassCompare::ClassCompare(SrcPos pos, Expr *lhs, Expr *rhs, Bool negate) :
			Expr(pos), lhs(lhs), rhs(rhs), negate(negate) {

			Value r = lhs->result().type();
			if ((r.type->typeFlags & typeClass) != typeClass)
				throw TypeError(lhs->pos, L"The default comparison operator can not be used with other types than classes.");

			r = rhs->result().type();
			if ((r.type->typeFlags & typeClass) != typeClass)
				throw TypeError(rhs->pos, L"The default comparison operator can not be used with other types than classes.");
		}

		ExprResult ClassCompare::result() {
			return ExprResult(Value(StormInfo<Bool>::type(engine())));
		}

		void ClassCompare::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			Bool needed = to->needed();

			CodeResult *l = needed
				? new (this) CodeResult(lhs->result().type().asRef(false), s->block)
				: new (this) CodeResult();
			lhs->code(s, l);

			CodeResult *r = needed
				? new (this) CodeResult(rhs->result().type().asRef(false), s->block)
				: new (this) CodeResult();
			rhs->code(s, r);

			// Is our result needed?
			if (!to->needed())
				return;

			VarInfo lVar = l->location(s);
			VarInfo rVar = r->location(s);
			VarInfo res = to->location(s);

			*s->l << cmp(lVar.v, rVar.v);
			*s->l << setCond(res.v, negate ? ifNotEqual : ifEqual);
			res.created(s);
		}

		void ClassCompare::toS(StrBuf *to) const {
			*to << lhs << S(" is ") << rhs;
		}



		/**
		 * Look up a proper action from a name and a set of parameters.
		 */
		static Expr *findCtor(Scope scope, Type *t, Actuals *actual, const SrcPos &pos);
		static Expr *findTarget(Scope scope, Named *n, LocalVar *first, Actuals *actual, const SrcPos &pos, bool useLookup);
		static Expr *findTargetThis(Block *block, SimpleName *name,
									Actuals *params, const SrcPos &pos,
									Named *&candidate);
		static Expr *findTarget(Block *block, SimpleName *name,
								const SrcPos &pos, Actuals *params,
								bool useThis);

		// Find a constructor.
		static Expr *findCtor(Scope scope, Type *t, Actuals *actual, const SrcPos &pos) {
			BSNamePart *part = new (t) BSNamePart(Type::CTOR, pos, actual);
			part->insert(thisPtr(t));

			Function *ctor = as<Function>(t->find(part, scope));
			if (!ctor)
				throw SyntaxError(pos, L"No constructor " + ::toS(t->identifier()) + L"." + ::toS(part) + L")");

			return new (t) CtorCall(pos, scope, ctor, actual);
		}


		// Helper to create the actual type, given something found. If '!useLookup', then we will not use the lookup
		// of the function or variable (ie use vtables).
		static Expr *findTarget(Scope scope, Named *n, LocalVar *first, Actuals *actual, const SrcPos &pos, bool useLookup) {
			if (!n)
				return null;

			if (wcscmp(n->name->c_str(), Type::CTOR) == 0)
				throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Type() instead.");
			if (wcscmp(n->name->c_str(), Type::DTOR) == 0)
				throw SyntaxError(pos, L"Manual invocations of destructors are forbidden.");

			if (Function *f = as<Function>(n)) {
				if (first)
					actual->addFirst(new (first) LocalVarAccess(pos, first));
				return new (n) FnCall(pos, scope, f, actual, useLookup, first ? true : false);
			}

			if (LocalVar *v = as<LocalVar>(n)) {
				assert(!first);
				return new (n) LocalVarAccess(pos, v);
			}

			if (MemberVar *v = as<MemberVar>(n)) {
				if (first)
					return new (n) MemberVarAccess(pos, new (first) LocalVarAccess(pos, first), v);
				else
					return new (n) MemberVarAccess(pos, actual->expressions->at(0), v);
			}

			if (NamedThread *v = as<NamedThread>(n)) {
				assert(!first);
				return new (n) NamedThreadAccess(pos, v);
			}

			return null;
		}

		static bool isSuperName(SimpleName *name) {
			if (name->count() != 2)
				return false;

			SimplePart *p = name->at(0);
			if (wcscmp(p->name->c_str(), S("super")) != 0)
				return false;
			return p->params->empty();
		}

		// Find a target assuming we should use the this-pointer.
		static Expr *findTargetThis(Block *block, SimpleName *name,
											Actuals *params, const SrcPos &pos,
											Named *&candidate) {
			const Scope &scope = block->scope;

			SimplePart *thisPart = new (block) SimplePart(new (block) Str(S("this")));
			LocalVar *thisVar = block->variable(thisPart);
			if (!thisVar)
				return null;

			BSNamePart *lastPart = new (name) BSNamePart(name->last()->name, pos, params);
			lastPart->insert(thisVar->result);
			bool useLookup = true;

			if (isSuperName(name)) {
				SimpleName *part = name->from(1);
				// It is something in the super type!
				Type *super = thisVar->result.type->super();
				if (!super)
					throw SyntaxError(pos, L"No super type for " + ::toS(thisVar->result) + L", can not use 'super' here.");

				part->last() = lastPart;
				candidate = storm::find(block->scope, super, part);
				useLookup = false;
			} else {
				// May be anything.
				name->last() = lastPart;
				candidate = scope.find(name);
				useLookup = true;
			}

			Expr *e = findTarget(block->scope, candidate, thisVar, params, pos, useLookup);
			if (e)
				return e;

			return null;
		}

		// Find whatever is meant by the 'name' in this context. Return suitable expression. If
		// 'useThis' is true, a 'this' pointer may be inserted as the first parameter.
		static Expr *findTarget(Block *block, SimpleName *name,
										const SrcPos &pos, Actuals *params,
										bool useThis) {
			const Scope &scope = block->scope;

			// Type ctors and local variables have priority.
			{
				Named *n = scope.find(name);
				if (Type *t = as<Type>(n))
					return findCtor(block->scope, t, params, pos);
				else if (as<LocalVar>(n) != null && params->empty())
					return findTarget(block->scope, n, null, params, pos, false);
			}

			// If we have a this-pointer, try to use it!
			Named *candidate = null;
			if (useThis)
				if (Expr *e = findTargetThis(block, name, params, pos, candidate))
					return e;

			// Try without the this pointer first.
			BSNamePart *last = new (name) BSNamePart(name->last()->name, pos, params);
			name->last() = last;
			Named *n = scope.find(name);

			if (Expr *e = findTarget(block->scope, n, null, params, pos, true))
				return e;

			if (!n && !candidate)
				throw SyntaxError(pos, L"Can not find " + ::toS(name) + L".");

			if (!n)
				n = candidate;

			throw TypeError(pos, ::toS(n) + L" is a " + ::toS(runtime::typeOf(n)->identifier()) +
							L". Only functions, variables and constructors are supported.");
		}

		Expr *namedExpr(Block *block, syntax::SStr *name, Actuals *params) {
			SimpleName *n = new (name) SimpleName(name->v);
			return findTarget(block, n, name->pos, params, true);
		}

		Expr *namedExpr(Block *block, SrcName *name, Actuals *params) {
			return namedExpr(block, name->pos, name, params);
		}

		Expr *namedExpr(Block *block, SrcPos pos, Name *name, Actuals *params) {
			SimpleName *simple = name->simplify(block->scope);
			if (!simple)
				throw SyntaxError(pos, L"Could not resolve parameters in " + ::toS(name));

			return findTarget(block, simple, pos, params, true);
		}

		Expr *namedExpr(Block *block, syntax::SStr *name, Expr *first, Actuals *params) {
			params->addFirst(first);
			SimpleName *n = new (name) SimpleName(name->v);
			return findTarget(block, n, name->pos, params, false);
		}

		Expr *namedExpr(Block *block, syntax::SStr *name, Expr *first) {
			Actuals *params = new (block) Actuals();
			params->add(first);
			SimpleName *n = new (name) SimpleName(name->v);
			return findTarget(block, n, name->pos, params, false);
		}

		Expr *STORM_FN spawnExpr(Expr *expr) {
			FnCall *fnCall = as<FnCall>(expr);
			if (!fnCall)
				throw SyntaxError(expr->pos, L"The spawn-syntax is not applicable to anything but functions"
								L" at the moment. This is a " + ::toS(runtime::typeOf(expr)->identifier()));

			fnCall->makeAsync();

			return expr;
		}


	}
}
