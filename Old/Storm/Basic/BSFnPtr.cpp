#include "stdafx.h"
#include "BSFnPtr.h"
#include "Lib/FnPtr.h"
#include "Function.h"
#include "Exception.h"
#include "Type.h"
#include "Engine.h"

namespace storm {
	namespace bs {
		// Note: We do not (yet?) support implicit this pointer.
		Function *findTarget(const Scope &scope, Par<SrcName> name, Par<ArrayP<SrcName>> formal, Par<Expr> dot) {
			Auto<SimpleName> resolved = name->simplify(scope);

			vector<Value> params;
			if (dot)
				params.push_back(dot->result().type());
			for (nat i = 0; i < formal->count(); i++)
				params.push_back(scope.value(formal->at(i)));

			Auto<SimplePart> last = CREATE(SimplePart, resolved, resolved->lastName(), params);
			resolved->last() = last;

			Auto<Named> found = scope.find(resolved);
			if (!found)
				throw SyntaxError(name->pos, L"Could not find " + ::toS(resolved));
			Auto<Function> fn = found.as<Function>();
			if (!fn)
				throw SyntaxError(name->pos, L"Can not take the pointer of anything else than a function, this is "
								+ ::toS(*found) + L"!");
			return fn.ret();
		}

	}

	bs::FnPtr *bs::strongFnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<SrcName>> formal) {
		return CREATE(FnPtr, block, block, dot, name, formal, true);
	}

	bs::FnPtr *bs::weakFnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<SrcName>> formal) {
		return CREATE(FnPtr, block, block, dot, name, formal, false);
	}

	bs::FnPtr::FnPtr(Par<Block> block, Par<SrcName> name, Par<ArrayP<SrcName>> formal)
		: Expr(name->pos), strongThis(false) {

		target = findTarget(block->scope, name, formal, null);
	}

	bs::FnPtr::FnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<SrcName>> formal, Bool strong)
		: Expr(name->pos), dotExpr(dot), strongThis(strong) {

		if (dotExpr->result().type().isValue())
			throw SyntaxError(dotExpr->pos, L"Only classes and actors can be bound to a function pointer. Not values.");

		Auto<SrcName> tn = CREATE(SrcName, this);
		tn->add(steal(CREATE(SimplePart, this, name->v)));
		target = findTarget(block->scope, tn, formal, dot);
	}

	ExprResult bs::FnPtr::result() {
		// TODO: Parameters should be taken from 'formal'. Consider when a pointer wants to restrict
		// a parameter to a derived class.
		vector<Value> params = target->params;
		if (dotExpr) {
			params[0] = target->result;
		} else {
			params.insert(params.begin(), target->result);
		}
		return Value::thisPtr(fnPtrType(engine(), params));
	}

	void bs::FnPtr::code(Par<CodeGen> to, Par<CodeResult> r) {
		using namespace code;

		Value type = result().type();

		// Note: initialized to zero if not needed.
		VarInfo thisPtr = variable(to, type);

		if (dotExpr) {
			Auto<CodeResult> result = CREATE(CodeResult, this, type, thisPtr);
			dotExpr->code(to, result);
		}

		if (r->needed()) {
			Engine &e = engine();
			VarInfo z = r->location(to);
			RunOn runOn = target->runOn();
			bool memberFn = target->isMember();

			to->to << lea(ptrA, target->ref());
			to->to << fnParam(type.type->typeRef);
			to->to << fnParam(ptrA);
			if (runOn.state == RunOn::named) {
				to->to << fnParam(runOn.thread->ref());
			} else {
				to->to << fnParam(natPtrConst(0));
			}
			to->to << fnParam(thisPtr.v.v);
			to->to << fnParam(byteConst(strongThis ? 1 : 0));
			to->to << fnParam(byteConst(memberFn ? 1 : 0));
			to->to << fnCall(e.fnRefs.fnPtrCreate, retPtr());
			to->to << mov(z.v.v, ptrA);
			z.created(to);
		}
	}

	void bs::FnPtr::output(wostream &to) const {
		to << "&";
		if (dotExpr) {
			to << dotExpr;
			to << (strongThis ? L"." : L"->");
			to << target->name << L"(";
			join(to, target->params, L", ");
			to << L")";
		} else {
			to << target;
		}
	}

}