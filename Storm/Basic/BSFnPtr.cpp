#include "stdafx.h"
#include "BSFnPtr.h"
#include "Lib/FnPtr.h"
#include "Function.h"
#include "Exception.h"

namespace storm {
	namespace bs {
		// Note: We do not yet support implicit this pointer.
		Function *findTarget(const Scope &scope, Par<TypeName> name, Par<ArrayP<TypeName>> formal, Par<Expr> dot) {
			Auto<Name> resolved = name->toName(scope);

			vector<Value> params;
			if (dot)
				params.push_back(dot->result());
			for (nat i = 0; i < formal->count(); i++)
				params.push_back(formal->at(i)->resolve(scope));

			resolved = resolved->withParams(params);

			Named *found = scope.find(resolved);
			if (!found)
				throw SyntaxError(name->pos, L"Could not find " + ::toS(resolved));
			Function *fn = as<Function>(found);
			if (!fn)
				throw SyntaxError(name->pos, L"Can not take the pointer of anything else than a function, this is "
								+ ::toS(*found) + L"!");
			return fn;
		}

	}

	bs::FnPtr::FnPtr(Par<Block> block, Par<TypeName> name, Par<ArrayP<TypeName>> formal) {
		target = findTarget(block->scope, name, formal, null);
	}

	bs::FnPtr::FnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<TypeName>> formal) :
		dotExpr(dot) {
		Auto<TypeName> tn = CREATE(TypeName, this);
		tn->add(steal(CREATE(TypePart, this, name)));
		target = findTarget(block->scope, tn, formal, dot);
	}

	Value bs::FnPtr::result() {
		vector<Value> params = target->params;
		params.insert(params.begin(), Value(target->result));
		return Value::thisPtr(fnPtrType(engine(), params));
	}

	void bs::FnPtr::code(Par<CodeGen> state, Par<CodeResult> r) {
		PLN("Can not generate code for " << *this << " yet!");
		assert(false);
	}

	void bs::FnPtr::output(wostream &to) const {
		to << "&";
		if (dotExpr) {
			to << dotExpr << L"." << target->name << L"(";
			join(to, target->params, L", ");
			to << L")";
		} else {
			to << target;
		}
	}

}
