#include "stdafx.h"
#include "Adapter.h"
#include "CodeGen.h"
#include "Engine.h"

namespace storm {
	using namespace code;

	const void *makeRefParams(Function *wrap) {
		assert(wrap->result.returnInReg(), L"Currently, returning values is not supported.");

		CodeGen *s = new (wrap) CodeGen(wrap->runOn());

		Array<Var> *params = new (wrap) Array<Var>();
		for (Nat i = 0; i < wrap->params->count(); i++) {
			params->push(s->createParam(wrap->params->at(i).asRef(true)));
		}


		*s->l << prolog();

		for (Nat i = 0; i < wrap->params->count(); i++) {
			Value type = wrap->params->at(i);
			if (type.ref) {
				// Nothing special needs to be done...
				*s->l << fnParam(params->at(i));
			} else {
				// Use the copy-ctor.
				*s->l << fnParamRef(params->at(i), type.size(), type.copyCtor());
			}
		}

		*s->l << fnCall(wrap->ref(), wrap->result.valTypeRet());
		*s->l << epilog();
		*s->l << ret(wrap->result.valTypeRet());

		Binary *b = new (wrap) Binary(wrap->engine().arena(), s->l);
		return b->address();
	}

}
