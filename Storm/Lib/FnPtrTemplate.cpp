#include "stdafx.h"
#include "FnPtrTemplate.h"
#include "Engine.h"
#include "CloneEnv.h"
#include "FnPtr.h"
#include "Function.h"
#include "Exception.h"

namespace storm {

	static void CODECALL copyCtor(void *mem, Par<FnPtrBase> o) {
		new (mem) FnPtrBase(o);
	}

	static void CODECALL deepCopy(Par<FnPtrBase> me, Par<CloneEnv> env) {
		me->deepCopy(env);
	}

	static void CODECALL destroyPtr(FnPtrBase *me) {
		me->~FnPtrBase();
	}

	Bool CODECALL fnPtrNeedsCopy(FnPtrBase *me, TObject *first) {
		return me->needsCopy(first);
	}

	void CODECALL fnPtrCallRaw(FnPtrBase *b, void *output, BasicTypeInfo *type, code::FnParams *params, TObject *first) {
		return b->callRaw(output, *type, *params, first);
	}


	static void callPlainCode(Par<FnPtrType> type, Par<CodeGen> s, code::Variable par, vector<code::Value> params) {
		for (nat i = 0; i < params.size(); i++) {
			const Value &t = type->params[i + 1];
			addFnParam(s, par, t, params[i], false);
		}
	}

	static void callCopyCode(Par<FnPtrType> type, Par<CodeGen> s, code::Variable par, vector<code::Value> params) {
		for (nat i = 0; i < params.size(); i++) {
			const Value &t = type->params[i + 1];
			addFnParamCopy(s, par, t, params[i], false);
		}
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

		// Decide if the first parameter is a TObject, and pass either that or null to functions
		// that needs to know.
		code::Value firstTObject = natPtrConst(0);
		if (type->params.size() > 1 && type->params[1].type->isA(TObject::stormType(e)))
			firstTObject = params[0];

		// Create the FnParams object.
		Variable fnParams = createFnParams(s, type->params.size() - 1).v;

		// Should we clone the result?
		Variable needClone = s->frame.createByteVar(s->block.v);
		s->to << fnParam(thisParam);
		s->to << fnParam(firstTObject);
		s->to << fnCall(e.fnRefs.fnPtrCopy, Size::sByte);
		s->to << mov(needClone, al);

		// Handle parameters...
		Label doCopy = s->to.label();
		Label done = s->to.label();
		s->to << cmp(needClone, byteConst(1));
		s->to << jmp(doCopy, ifEqual);
		callPlainCode(type, s, fnParams, params);
		s->to << jmp(done);
		s->to << doCopy;
		callCopyCode(type, s, fnParams, params);
		s->to << done;

		// Return type info.
		Variable typeInfo = createBasicTypeInfo(s, result);

		// TODO: Do we need to copy the result?
		if (!result.returnInReg()) {
			s->to << lea(ptrB, typeInfo);
			s->to << lea(ptrC, fnParams);
			s->to << fnParam(thisParam);
			s->to << fnParam(resultParam);
			s->to << fnParam(ptrB);
			s->to << fnParam(ptrC);
			s->to << fnParam(firstTObject);
			s->to << fnCall(e.fnRefs.fnPtrCall, Size());

			// Need to copy?
			Label noCopy = s->to.label();
			s->to << cmp(needClone, byteConst(0));
			s->to << jmp(noCopy, ifEqual);

			// Call stdClone.
			Type *envType = CloneEnv::stormType(e);
			Variable cloneEnv = s->frame.createPtrVar(s->block.v, e.fnRefs.release);
			allocObject(s, envType->defaultCtor(), vector<code::Value>(), cloneEnv);
			Function *deepCopy = result.type->deepCopyFn();
			if (!deepCopy)
				throw InternalError(L"The type " + ::toS(result) + L" does not have the required 'deepCopy' member.");

			s->to << fnParam(resultParam);
			s->to << fnParam(cloneEnv);
			s->to << fnCall(deepCopy->ref(), Size::sPtr);

			s->to << noCopy;
			s->to << mov(ptrA, resultParam);
			s->to << epilog();
			s->to << ret(Size::sPtr);
		} else if (result.isClass()) {
			// If 'tmp' is refcounted, we do not need to add references. That is
			// already done by the callee!
			Variable tmp = s->frame.createVariable(s->block.v, Size::sPtr, e.fnRefs.release);
			s->to << lea(ptrA, tmp);
			s->to << lea(ptrB, typeInfo);
			s->to << lea(ptrC, fnParams);
			s->to << fnParam(thisParam);
			s->to << fnParam(ptrA);
			s->to << fnParam(ptrB);
			s->to << fnParam(ptrC);
			s->to << fnParam(firstTObject);
			s->to << fnCall(e.fnRefs.fnPtrCall, Size());

			// Need to copy?
			Label noCopy = s->to.label();
			Label done = s->to.label();
			s->to << cmp(needClone, byteConst(0));
			s->to << jmp(noCopy, ifEqual);

			// Copy.
			s->to << fnParam(tmp);
			s->to << fnCall(stdCloneFn(result).v, Size::sPtr);
			s->to << jmp(done);

			s->to << noCopy;
			s->to << code::addRef(tmp);
			s->to << mov(ptrA, tmp);

			s->to << done;
			s->to << epilog();
			s->to << ret(Size::sPtr);
		} else {
			// This is a primitive, we never have to copy it!
			Variable tmp = s->frame.createVariable(s->block.v, result.size());
			s->to << lea(ptrA, tmp);
			s->to << lea(ptrB, typeInfo);
			s->to << lea(ptrC, fnParams);
			s->to << fnParam(thisParam);
			s->to << fnParam(ptrA);
			s->to << fnParam(ptrB);
			s->to << fnParam(ptrC);
			s->to << fnParam(firstTObject);
			s->to << fnCall(e.fnRefs.fnPtrCall, Size());
			if (result.size() != Size()) // void
				s->to << mov(asSize(ptrA, result.size()), tmp);
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

		vector<Value> fnCall = params;
		fnCall[0] = t;
		add(steal(dynamicFunction(e, params[0], L"call", fnCall, callCode(this))));
		add(steal(nativeDtor(e, this, &destroyPtr)));
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
