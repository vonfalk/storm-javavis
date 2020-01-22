#include "stdafx.h"
#include "Clone.h"
#include "CodeGen.h"
#include "Type.h"
#include "Package.h"
#include "Engine.h"
#include "Exception.h"

namespace storm {
	using namespace code;

	CloneTemplate::CloneTemplate() : Template(new (engine()) Str(L"clone")) {}

	// Dummy for TObjects.
	static TObject *CODECALL returnPtr(TObject *o) {
		return o;
	}

	static TObject *CODECALL returnPtrEnv(TObject *o, CloneEnv *e) {
		return o;
	}

	static Function *generateSingle(Value type) {
		Engine &e = type.type->engine;
		Value cloneEnvT = Value(CloneEnv::stormType(e));
		Array<Value> *params = new (e) Array<Value>(1, type);

		if (type.isActor())
			return nativeFunction(e, type, S("clone"), params, address(&returnPtr));
		else if (type.isClass())
			return nativeFunction(e, type, S("clone"), params, address(&runtime::cloneObject));

		// Value or built-in type. We need to generate some code...
		CodeGen *g = new (e) CodeGen(RunOn(), false, type);
		Var in = g->createParam(type);

		*g->l << prolog();

		if (Function *copyFn = type.type->deepCopyFn()) {
			Var cloneEnv = allocObject(g, cloneEnvT.type);
			*g->l << lea(ptrA, in);
			*g->l << fnParam(e.ptrDesc(), ptrA);
			*g->l << fnParam(e.ptrDesc(), cloneEnv);
			*g->l << fnCall(copyFn->ref(), true);
		}

		g->returnValue(in);

		return dynamicFunction(e, type, S("clone"), params, g->l);
	}

	static Function *generateDual(Value type) {
		Engine &e = type.type->engine;
		Value cloneEnvT = Value(CloneEnv::stormType(e));
		Array<Value> *params = new (e) Array<Value>(2, type);
		params->at(1) = cloneEnvT;

		if (type.isActor())
			return nativeFunction(e, type, S("clone"), params, address(&returnPtrEnv));
		else if (type.isObject())
			return nativeFunction(e, type, S("clone"), params, address(&runtime::cloneObjectEnv));

		// Value or built-in type. We need to generate some code...
		CodeGen *g = new (e) CodeGen(RunOn(), false, type);
		Var in = g->createParam(type);
		Var cloneEnv = g->createParam(cloneEnvT);

		*g->l << prolog();

		if (Function *copyFn = type.type->deepCopyFn()) {
			*g->l << lea(ptrA, in);
			*g->l << fnParam(e.ptrDesc(), ptrA);
			*g->l << fnParam(e.ptrDesc(), cloneEnv);
			*g->l << fnCall(copyFn->ref(), true);
		}

		g->returnValue(in);

		return dynamicFunction(e, type, S("clone"), params, g->l);
	}

	MAYBE(Named *) CloneTemplate::generate(SimplePart *part) {
		Array<Value> *params = part->params;
		if (params->count() < 1 || params->count() > 2)
			return null;

		Value type = params->at(0).asRef(false);
		if (type.type == null)
			return null;

		Named *result = null;
		if (params->count() == 2) {
			Value cloneEnv = params->at(1).asRef(false);
			if (cloneEnv.type != CloneEnv::stormType(engine()))
				return null;

			result = generateDual(type);
		} else {
			result = generateSingle(type);
		}

		if (result)
			result->flags |= namedMatchNoInheritance;
		return result;
	}

	Function *cloneFn(Type *t) {
		Engine &e = t->engine;
		Package *p = e.package(S("core"));
		Function *r = as<Function>(p->find(S("clone"), Value(t), e.scope()));
		if (!r)
			throw new (t) InternalError(TO_S(t->engine, S("Can not finde core.clone for ") << t->identifier()));
		return r;
	}

	Function *cloneFnEnv(Type *t) {
		Engine &e = t->engine;
		Package *p = e.package(S("core"));
		Array<Value> *params = new (e) Array<Value>(2, Value(t));
		params->at(1) = Value(CloneEnv::stormType(e));

		Function *r = as<Function>(p->find(S("clone"), params, e.scope()));
		if (!r)
			throw new (t) InternalError(TO_S(t->engine, S("Can not finde core.clone for ") << t->identifier()));
		return r;
	}

}
