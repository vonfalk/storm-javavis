#include "stdafx.h"
#include "AsRef.h"
#include "Engine.h"
#include "Code/Listing.h"

namespace storm {

	static void addParam(code::Listing &to, const Value &param) {
		using namespace code;

		if (param.ref) {
			// Nothing to do, the target function already takes a reference!
			to << fnParam(to.frame.createPtrParam());
			return;
		}

		if (param.isClass()) {
			Variable tmp = to.frame.createPtrVar(to.frame.root());
			to << mov(ptrA, to.frame.createPtrParam());
			to << mov(tmp, ptrRel(ptrA));
			to << fnParam(tmp);
		} else {
			if (param.isBuiltIn()) {
				Variable tmp = to.frame.createVariable(to.frame.root(), param.size());
				to << mov(ptrA, to.frame.createPtrParam());
				to << mov(tmp, xRel(param.size(), ptrA));
				to << fnParam(tmp);
			} else {
				code::Value copy = param.copyCtor();
				Variable p = to.frame.createPtrParam();
				to << fnParamRef(p, copy);
			}
		}
	}

	static code::Listing generate(const Value &result, const ValList &params, const code::Ref &fn, bool member) {
		using namespace code;
		Listing to;

		bool stackResult = !result.returnInReg();

		to << prolog();

		nat from = 0;
		if (member)
			addParam(to, params[from++]);

		// Forward before the 'this' ptr if we're not a member function.
		if (stackResult) {
			to << fnParam(to.frame.createPtrParam());
		}

		for (nat i = from; i < params.size(); i++)
			addParam(to, params[i]);

		to << fnCall(fn, result.retVal());

		to << epilog();
		to << ret(result.retVal());

		return to;
	}

	AsRef::AsRef(code::Arena &arena, const Value &result, const ValList &params, const code::Ref &fn, bool isMember) :
		source(arena, fn.targetName() + L"-refwrap") {
		source.set(new code::Binary(arena, generate(result, params, fn, isMember)));
	}

	AsRef::AsRef(Par<Function> fn) :
		source(fn->engine().arena, fn->identifier() + L"-refwrap") {
		source.set(new code::Binary(fn->engine().arena, generate(fn->result, fn->params, fn->ref(), fn->isMember())));
	}

}
