#include "stdafx.h"
#include "FnPtr.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Fn.h"

namespace storm {
	namespace bs {

		// Note: We do not (yet?) support implicit this pointer.
		static Function *findTarget(const Scope &scope, SrcName *name, Array<SrcName *> *formal, Expr *dot) {
			SimpleName *resolved = name->simplify(scope);

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
			Array<Value> *params = target->params;
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

			if (r->needed()) {
				assert(false, L"TODO: Implement me!");
				// Engine &e = engine();
				// VarInfo z = r->location(to);
				// RunOn runOn = target->runOn();
				// bool memberFn = target->isMember();

				// to->to << lea(ptrA, target->ref());
				// to->to << fnParam(type.type->typeRef);
				// to->to << fnParam(ptrA);
				// if (runOn.state == RunOn::named) {
				// 	to->to << fnParam(runOn.thread->ref());
				// } else {
				// 	to->to << fnParam(natPtrConst(0));
				// }
				// to->to << fnParam(thisPtr.v.v);
				// to->to << fnParam(byteConst(strongThis ? 1 : 0));
				// to->to << fnParam(byteConst(memberFn ? 1 : 0));
				// to->to << fnCall(e.fnRefs.fnPtrCreate, retPtr());
				// to->to << mov(z.v.v, ptrA);
				// z.created(to);
			}
		}

		void FnPtr::toS(StrBuf *to) const {
			*to << "&";
			if (dotExpr) {
				*to << dotExpr << L".";
				*to << target->name << L"(";
				join(to, target->params, L", ");
				*to << L")";
			} else {
				*to << target;
			}
		}


	}
}
