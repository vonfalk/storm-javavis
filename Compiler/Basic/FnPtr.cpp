#include "stdafx.h"
#include "FnPtr.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Fn.h"
#include "Core/Join.h"

namespace storm {
	namespace bs {

		// Note: We do not (yet?) support implicit this pointer.
		static Function *findTarget(const Scope &scope, SrcName *name, Array<SrcName *> *formal, Expr *dot) {
			SimpleName *resolved = name->simplify(scope);
			if (!resolved)
				throw SyntaxError(name->pos, L"The parameters of the name " + ::toS(name) +
								L" can not be resolved to proper types.");

			Array<Value> *params = new (name) Array<Value>();
			if (dot)
				params->push(dot->result().type());
			for (nat i = 0; i < formal->count(); i++)
				params->push(scope.value(formal->at(i)));

			SimplePart *last = new (resolved) SimplePart(resolved->last()->name, params);
			resolved->last() = last;

			Named *found = scope.find(resolved);
			if (!found)
				throw SyntaxError(name->pos, L"Could not find " + ::toS(resolved));
			Function *fn = as<Function>(found);
			if (!fn)
				throw SyntaxError(name->pos, L"Can not take the pointer of anything else than a function, this is "
								+ ::toS(*found) + L"!");
			return fn;
		}


		FnPtr::FnPtr(Block *block, SrcName *name, Array<SrcName *>*formal) : Expr(name->pos) {
			target = findTarget(block->scope, name, formal, null);
		}

		FnPtr::FnPtr(Block *block, Expr *dot, syntax::SStr *name, Array<SrcName *>*formal)
			: Expr(name->pos), dotExpr(dot) {

			if (dotExpr->result().type().isValue())
				throw SyntaxError(dotExpr->pos, L"Only classes and actors can be bound to a function pointer. Not values.");

			SrcName *tn = CREATE(SrcName, this);
			tn->add(new (this) SimplePart(name->v));
			target = findTarget(block->scope, tn, formal, dot);
		}

		ExprResult FnPtr::result() {
			// TODO: Parameters should be taken from 'formal'. Consider when a pointer wants to restrict
			// a parameter to a derived class.
			Array<Value> *params = clone(target->params);
			if (dotExpr) {
				params->at(0) = target->result;
			} else {
				params->insert(0, target->result);
			}
			return thisPtr(fnType(params));
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
