#include "stdafx.h"
#include "FnPtrTemplate.h"
#include "Engine.h"
#include "CloneEnv.h"
#include "FnPtr.h"
#include "Function.h"

namespace storm {

	static void CODECALL copyCtor(void *mem, Par<FnPtrBase> o) {
		new (mem) FnPtrBase(o);
	}

	static void CODECALL deepCopy(Par<FnPtrBase> me, Par<CloneEnv> env) {
		me->deepCopy(env);
	}

	Bool CODECALL fnPtrNeedsCopy(FnPtrBase *me) {
		return me->needsCopy();
	}

	static code::Listing callCode(Par<FnPtrType> type) {
		using namespace code;
		Engine &e = type->engine;
		Auto<CodeGen> s = CREATE(CodeGen, type, RunOn());
		s->to << prolog();

		// This parameter.
		Variable thisParam = s->frame.createPtrParam();

		// Result parameter.
		const Value &result = type->params[0];
		Variable resultParam;
		if (!result.returnInReg()) {
			resultParam = s->frame.createPtrParam();
		}

		// Create parameters.
		vector<code::Value> params(type->params.size() - 1);
		for (nat i = 0; i < params.size(); i++) {
			const Value &t = type->params[i + 1];
			if (t.isClass())
				params[i] = s->frame.createPtrParam();
			else
				params[i] = s->frame.createParameter(t.size(), false, t.destructor());
		}

		// Should we clone the result?
		Variable needClone = s->frame.createByteVar(s->block.v);
		s->to << fnParam(thisParam);
		s->to << fnCall(e.fnRefs.fnPtrCopy, Size::sByte);
		s->to << mov(needClone, al);

		// Handle parameters...

		if (!result.returnInReg()) {
			s->to << mov(ptrA, resultParam);
			s->to << epilog();
			s->to << ret(Size::sPtr);
		} else {
			s->to << epilog();
			s->to << ret(result.size());
		}

		return s->to;
	}

	FnPtrType::FnPtrType(const vector<Value> &v) : Type(L"FnPtr", typeClass, v) {
		setSuper(FnPtrBase::stormType(engine));
		matchFlags = matchNoInheritance;
	}

	void FnPtrType::lazyLoad() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value::thisPtr(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&storm::copyCtor))));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&storm::deepCopy))));

		vector<Value> rest(params.begin() + 1, params.end());
		add(steal(dynamicFunction(e, params[0], L"call", rest, callCode(this))));
	}

	static Named *generateFnPtr(Par<NamePart> part) {
		const vector<Value> &params = part->params;

		if (params.size() < 1)
			return null;

		for (nat i = 0; i < params.size(); i++) {
			// References not allowed.
			if (params[i].ref)
				return null;
		}

		Engine &e = part->engine();
		Type *r = CREATE(FnPtrType, e, params);
		return r;
	}

	void addFnPtrTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"FnPtr", simpleFn(&generateFnPtr));
		to->add(t);
	}

	Type *fnPtrType(Engine &e, const vector<Value> &params) {
		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"FnPtr", params);
		Type *r = as<Type>(e.scope()->find(tName));
		assert(r, "The FnPtr type was not found!");
		return r;
	}


}
