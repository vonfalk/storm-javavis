#include "stdafx.h"
#include "FnPtr.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Fn.h"
#include "Core/Join.h"

namespace storm {
	namespace bs {

		FnPtr::FnPtr(Block *block, SrcName *name, Array<SrcName *> *formal) : Expr(name->pos) {
			findTarget(block->scope, name, formal, null);
		}

		FnPtr::FnPtr(Block *block, Expr *dot, syntax::SStr *name, Array<SrcName *>*formal)
			: Expr(name->pos), dotExpr(dot) {

			Value dotResult = dotExpr->result().type();
			if (dotResult.isValue() || dotResult.isBuiltIn())
				throw SyntaxError(dotExpr->pos, L"Only classes and actors can be bound to a function pointer. Not values.");

			SrcName *tn = CREATE(SrcName, this);
			tn->add(new (this) SimplePart(name->v));
			findTarget(block->scope, tn, formal, dot);
		}

		FnPtr::FnPtr(Function *target, SrcPos pos) : Expr(pos), target(target) {
			Array<Value> *formals = clone(target->params);
			formals->insert(0, target->result);
			ptrType = thisPtr(fnType(formals));
		}

		FnPtr::FnPtr(Expr *dot, Function *target, SrcPos pos) : Expr(pos), dotExpr(dot), target(target) {
			if (dotExpr->result().type().isValue())
				throw SyntaxError(dotExpr->pos, L"Only classes and actors can be bound to a function pointer. Not values.");

			if (target->params->empty() || !target->params->at(0).canStore(dot->result().type()))
				throw SyntaxError(dotExpr->pos, L"The first parameter of the specified function does not match the type of the provided expression.");

			Array<Value> *formals = clone(target->params);
			formals->at(0) = target->result;
			ptrType = thisPtr(fnType(formals));
		}

		// Note: We don't support implicit this pointer.
		void FnPtr::findTarget(const Scope &scope, SrcName *name, Array<SrcName *> *formal, MAYBE(Expr *) dot) {
			SimpleName *resolved = name->simplify(scope);
			if (!resolved)
				throw SyntaxError(name->pos, L"The parameters of the name " + ::toS(name) +
								L" can not be resolved to proper types.");

			Array<Value> *params = new (this) Array<Value>();
			Array<Value> *formals = new (this) Array<Value>();
			if (dot)
				params->push(dot->result().type());
			for (Nat i = 0; i < formal->count(); i++) {
				Value v = scope.value(formal->at(i));
				params->push(v);
				formals->push(v);
			}

			SimplePart *last = new (resolved) SimplePart(resolved->last()->name, params);
			resolved->last() = last;

			Named *found = scope.find(resolved);
			if (!found)
				throw SyntaxError(name->pos, L"Could not find " + ::toS(resolved));
			target = as<Function>(found);
			if (!target)
				throw SyntaxError(name->pos, L"Can not take the pointer of anything other than a function. This is a " +
								::toS(*found) + L"!");

			formals->insert(0, target->result);
			ptrType = thisPtr(fnType(formals));
		}

		ExprResult FnPtr::result() {
			return ptrType;
		}

		void FnPtr::code(CodeGen *to, CodeResult *r) {
			using namespace code;

			Value type = result().type();

			// Note: initialized to zero if not needed.
			VarInfo thisPtr = to->createVar(type);

			if (dotExpr) {
				CodeResult *result = new (this) CodeResult(type, thisPtr);
				dotExpr->code(to, result);
			}

			if (!r->needed())
				return;

			VarInfo z = r->location(to);
			if (!dotExpr) {
				// Create the object once and store it.
				FnBase *obj = pointer(target);

				*to->l << mov(z.v, objPtr(obj));
			} else {
				// We need to create a new object each time since the 'dotExpr' might change.
				RunOn runOn = target->runOn();
				bool memberFn = target->isMember();
				code::TypeDesc *ptr = engine().ptrDesc();

				*to->l << lea(ptrA, target->ref());
				*to->l << fnParam(ptr, type.type->typeRef());
				*to->l << fnParam(ptr, ptrA);
				if (runOn.state == RunOn::named)
					*to->l << fnParam(ptr, runOn.thread->ref());
				else
					*to->l << fnParam(ptr, ptrConst(Offset()));
				*to->l << fnParam(ptr, thisPtr.v);
				*to->l << fnParam(byteDesc(engine()), byteConst(memberFn ? 1 : 0));
				*to->l << fnCall(engine().ref(Engine::rFnCreate), false, ptr, ptrA);
				*to->l << mov(z.v, ptrA);
			}
			z.created(to);
		}

		void FnPtr::toS(StrBuf *to) const {
			*to << S("&");
			if (dotExpr) {
				*to << dotExpr << S(".");
				*to << target->name << S("(") << join(target->params, S(", ")) << S(")");
			} else {
				*to << target;
			}
		}


	}
}
