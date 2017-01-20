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
			return nativeFunction(e, type, L"clone", params, address(&returnPtr));
		else if (type.isHeapObj())
			return nativeFunction(e, type, L"clone", params, address(&runtime::cloneObject));

		// Value or built-in type. We need to generate some code...
		CodeGen *g = new (e) CodeGen(RunOn());
		Var in = g->createParam(type);
		g->result(type, false);

		*g->l << prolog();

		Array<Value> *deepParams = valList(e, 2, thisPtr(type.type), cloneEnvT);
		if (Function *copyFn = as<Function>(type.type->find(L"deepCopy", deepParams))) {
			Var cloneEnv = allocObject(g, cloneEnvT.type);
			*g->l << lea(ptrA, in);
			*g->l << fnParam(ptrA);
			*g->l << fnParam(cloneEnv);
			*g->l << fnCall(copyFn->ref(), valVoid());
		}

		g->returnValue(in);

		return dynamicFunction(e, type, L"clone", params, g->l);
	}

	static Function *generateDual(Value type) {
		Engine &e = type.type->engine;
		Value cloneEnvT = Value(CloneEnv::stormType(e));
		Array<Value> *params = new (e) Array<Value>(2, type);
		params->at(1) = cloneEnvT;

		if (type.isActor())
			return nativeFunction(e, type, L"clone", params, address(&returnPtrEnv));
		else if (type.isHeapObj())
			return nativeFunction(e, type, L"clone", params, address(&runtime::cloneObjectEnv));

		// Value or built-in type. We need to generate some code...
		CodeGen *g = new (e) CodeGen(RunOn());
		Var in = g->createParam(type);
		Var cloneEnv = g->createParam(cloneEnvT);
		g->result(type, false);

		*g->l << prolog();

		Array<Value> *deepParams = valList(e, 2, thisPtr(type.type), cloneEnvT);
		if (Function *copyFn = as<Function>(type.type->find(L"deepCopy", deepParams))) {
			*g->l << lea(ptrA, in);
			*g->l << fnParam(ptrA);
			*g->l << fnParam(cloneEnv);
			*g->l << fnCall(copyFn->ref(), valVoid());
		}

		g->returnValue(in);

		return dynamicFunction(e, type, L"clone", params, g->l);
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
		Package *p = t->engine.package(L"core");
		Function *r = as<Function>(p->find(L"clone", Value(t)));
		if (!r)
			throw InternalError(L"Can not finde core.clone for " + ::toS(t->identifier()));
		return r;
	}

}
