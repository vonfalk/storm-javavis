#include "stdafx.h"
#include "Named.h"
#include "Exception.h"
#include "Cast.h"
#include "Lookup.h"
#include "Actuals.h"
#include "Block.h"
#include "Resolve.h"
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
			} else if (!to->type().isAsmType()) {
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

		UnresolvedName *FnCall::name() {
			// This is approximate, and assumes that if we have a getter, the setter must be located
			// at the same place (which is actually fairly reasonable in many cases). This is,
			// however, not always true.

			BlockLookup *lookup = as<BlockLookup>(scope.top);
			if (!lookup)
				throw InternalError(L"Can not use FnCall::name() without having a scope referring to a block.");
			return new (this) UnresolvedName(lookup->block, toExecute->path(), pos, params, false);
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
				*to << S("&");
		}

		/**
		 * Execute constructor.
		 */

		CtorCall::CtorCall(SrcPos pos, Scope scope, Function *ctor, Actuals *params)
			: Expr(pos), ctor(ctor), params(params), scope(scope) {

			assert(params->expressions->count() == ctor->params->count() - 1, L"Invalid number of parameters to constructor!");
			toCreate = ctor->params->at(0).asRef(false);

			// Throws an error if we should not instantiate the type.
			toCreate.type->ensureNonAbstract(pos);
		}

		ExprResult CtorCall::result() {
			return toCreate.asRef(false);
		}

		void CtorCall::toS(StrBuf *to) const {
			*to << toCreate << params;
		}

		void CtorCall::code(CodeGen *s, CodeResult *to) {
			if (toCreate.isValue())
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
				if (!var->result.isAsmType()) {
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
				if (!type.isAsmType()) {
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
			*to << type << S("<bare>") << code::Operand(var);
		}


		/**
		 * Member variable.
		 */

		MemberVarAccess::MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var, Bool sameObject)
			: Expr(pos), member(member), var(var), assignTo(false), sameObject(sameObject) {}

		ExprResult MemberVarAccess::result() {
			return var->type.asRef();
		}

		void MemberVarAccess::assignResult() {
			assignTo = true;
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
			RunOn target = var->owner()->runOn();
			if (sameObject || s->runOn.canRun(target)) {
				extractPlainCode(s, to);
				return;
			}

			// We need to copy. See if we shall warn the user...
			if (assignTo)
				throw SyntaxError(pos, L"Unable to assign to member variables in objects running "
								L"on a different thread than the caller. Use assignment functions "
								L"for this task instead.");

			extractCopyCode(s, to);
		}

		void MemberVarAccess::extractPlainCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			VarInfo result = to->location(s);
			if (to->type().ref) {
				*s->l << add(ptrA, ptrConst(var->offset()));
				*s->l << mov(result.v, ptrA);
			} else if (!to->type().isAsmType()) {
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

		void MemberVarAccess::extractCopyCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			Value type = to->type();
			VarInfo result = to->location(s);
			VarInfo local = result;
			if (type.ref) {
				// We need to create a value here that we can use later.
				local = s->createVar(type.asRef(false));
			}

			Function *fn = type.type->readRefFn();

			// Figure out which thread is the proper one. We can not use Function::findThread since
			// that would find the thread for the *function* rather than the object we're reading from.
			code::Var thread = s->l->createVar(s->block, Size::sPtr);
			RunOn runOn = var->owner()->runOn();
			switch (runOn.state) {
			case RunOn::runtime:
				*s->l << mov(ptrC, ptrA);
				*s->l << add(ptrC, engine().ref(Engine::rTObjectOffset));
				*s->l << mov(thread, ptrRel(ptrC, Offset()));
				break;
			case RunOn::named:
				*s->l << mov(thread, runOn.thread->ref());
				break;
			default:
				throw InternalError(L"Can not issue a threaded read on a non-threaded object.");
			}

			// Offset the pointer and continue. Note: We do not want to modify 'obj' since that
			// could be used as eg. a local variable.
			code::Var data = s->l->createVar(s->block, Size::sPtr);
			*s->l << add(ptrA, ptrConst(var->offset()));
			*s->l << mov(data, ptrA);

			// Call the function on the proper thread.
			Array<Operand> *params = new (this) Array<Operand>(1, Operand(data));
			fn->threadCall(s, params, new (this) CodeResult(type.asRef(false), local), thread);

			if (type.ref) {
				*s->l << lea(result.v, local.v);
			}
		}

		void MemberVarAccess::toS(StrBuf *to) const {
			*to << member << S(".") << var->name;
		}


		/**
		 * Global variable.
		 */

		GlobalVarAccess::GlobalVarAccess(SrcPos pos, GlobalVar *var) : Expr(pos), var(var) {}

		ExprResult GlobalVarAccess::result() {
			return var->type.asRef();
		}

		void GlobalVarAccess::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			// Check so that we're using the proper thread!
			if (!var->accessibleFrom(s->runOn))
				throw SyntaxError(pos, L"Can not access the global variable " + ::toS(var) +
								L" from a thread other than " + ::toS(var->owner));

			if (!to->needed())
				return;


			*s->l << fnParam(engine().ptrDesc(), objPtr(var));
			*s->l << fnCall(engine().ref(Engine::rGlobalAddr), true, engine().ptrDesc(), ptrA);

			if (to->type().ref) {
				*s->l << mov(to->location(s).v, ptrA);
			} else {
				// We need to make a copy...
				VarInfo d = to->location(s);
				if (!var->type.isAsmType()) {
					*s->l << lea(ptrC, d.v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), ptrA);
					*s->l << fnCall(var->type.copyCtor(), true);
				} else {
					*s->l << mov(d.v, xRel(d.v.size(), ptrA, Offset()));
				}
			}
		}

		void GlobalVarAccess::toS(StrBuf *to) const {
			*to << var->identifier();
		}


		/**
		 * Named thread.
		 */

		NamedThreadAccess::NamedThreadAccess(SrcPos pos, NamedThread *thread) : Expr(pos), thread(thread) {}

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
			*to << to << S(" = ") << value;
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


		Expr *STORM_FN spawnExpr(Expr *expr) {
			// If it is an unresolved name: poke it so that it throws the more appropriate error
			// rather than us delivering a less informative error.
			if (as<UnresolvedName>(expr))
				expr->result();

			FnCall *fnCall = as<FnCall>(expr);
			if (!fnCall)
				throw SyntaxError(expr->pos, L"The spawn-syntax is not applicable to anything but functions"
								L" at the moment. This is a " + ::toS(runtime::typeOf(expr)->identifier()));

			fnCall->makeAsync();

			return expr;
		}


	}
}
