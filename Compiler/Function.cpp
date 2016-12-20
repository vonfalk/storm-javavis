#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "Exception.h"
#include "Code.h"
#include "Core/Str.h"
#include "Lib/Future.h"

namespace storm {

	Function::Function(Value result, Str *name, Array<Value> *params) :
		Named(name, params),
		result(result),
		lookupRef(null),
		codeRef(null),
		runOnThread(null) {}

	const void *Function::pointer() {
		return ref().address();
	}

	code::Ref Function::ref() {
		initRefs();
		return code::Ref(lookupRef);
	}

	code::Ref Function::directRef() {
		initRefs();
		return code::Ref(codeRef);
	}

	Bool Function::isMember() {
		return as<Type>(parent()) != null;
	}

	RunOn Function::runOn() const {
		if (Type *t = as<Type>(parent())) {
			return t->runOn();
		} else if (runOnThread) {
			return RunOn(runOnThread);
		} else {
			return RunOn(RunOn::any);
		}
	}

	void Function::runOn(NamedThread *t) {
		runOnThread = t;
	}

	void Function::toS(StrBuf *to) const {
		*to << result << L" ";
		Named::toS(to);
	}

	void Function::setCode(Code *code) {
		if (this->code)
			this->code->detach();
		code->attach(this);
		this->code = code;
		if (codeRef)
			this->code->update(codeRef);
	}

	Code *Function::getCode() const {
		return code;
	}

	void Function::setLookup(MAYBE(Code *) code) {
		if (lookup)
			lookup->detach();

		if (code == null && codeRef != null)
			lookup = new (this) DelegatedCode(code::Ref(codeRef));
		else
			lookup = code;

		if (lookup) {
			lookup->attach(this);
			if (lookupRef)
				lookup->update(lookupRef);
		}
	}

	void Function::compile() {
		if (code)
			code->compile();
		if (lookup)
			lookup->compile();
	}

	void Function::initRefs() {
		if (!codeRef) {
			assert(parent(), L"Too early!");
			codeRef = new (this) code::RefSource(*identifier() + L"<c>");
			if (code)
				code->update(codeRef);
		}

		if (!lookupRef) {
			assert(parent(), L"Too early!");
			lookupRef = new (this) code::RefSource(*identifier() + L"<l>");
			if (!lookup) {
				lookup = new (this) DelegatedCode(code::Ref(codeRef));
				lookup->attach(this);
			}
			lookup->update(lookupRef);
		}
	}

	/**
	 * Function calls.
	 */

	code::Var Function::findThread(CodeGen *s, Array<code::Operand> *params) {
		using namespace code;

		RunOn on = runOn();
		assert(on.state != RunOn::any, L"Only use 'findThread' on functions which 'runOn()' something other than any.");

		Var r = s->to->createVar(s->block, Size::sPtr);

		switch (on.state) {
		case RunOn::runtime:
			// Should be a this-ptr. Does not work well for constructors.
			assert(wcscmp(name->c_str(), Type::CTOR) == 0,
				L"Please overload 'findThread' your constructor '" + ::toS(identifier()) + L"'!");
			*s->to << mov(ptrA, params->at(0));
			*s->to << add(ptrA, engine().ref(Engine::rTObjectOffset));
			*s->to << mov(r, ptrRel(ptrA, Offset()));
			break;
		case RunOn::named:
			*s->to << mov(r, on.thread->ref());
			break;
		default:
			assert(false, L"Unknown state.");
			break;
		}

		return r;
	}

	void Function::autoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		RunOn r = runOn();
		if (to->runOn.canRun(r)) {
			localCall(to, params, result, true);
		} else {
			code::Var v = findThread(to, params);
			threadCall(to, params, result, v);
		}
	}

	void Function::localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, Bool useLookup) {
		initRefs();
		if (params->count() != this->params->count())
			throw InternalError(L"Parameter count mismatch when calling " + ::toS(identifier()));

		InlineCode *inlined = as<InlineCode>(code);
		// If we're not going to use the lookup, we may choose to inline sooner.
		if (useLookup && as<DelegatedCode>(lookup) == null)
			inlined = null;

		if (inlined)
			inlined->code(to, params, result);
		else
			localCall(to, params, result, useLookup ? this->ref() : directRef());
	}

	void Function::localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *res, code::Ref ref) {
		using namespace code;

		if (result == Value()) {
			addParams(to, params, code::Var());

			*to->to << fnCall(ref, valVoid());
		} else {
			VarInfo rVar = res->safeLocation(to, result);
			addParams(to, params, rVar.v);

			if (result.returnInReg()) {
				*to->to << fnCall(ref, result.valType());
				*to->to << mov(rVar.v, asSize(ptrA, result.size()));
			} else {
				// Ignore return value...
				*to->to << fnCall(ref, valVoid());
			}

			rVar.created(to);
		}
	}

	void Function::addParams(CodeGen *to, Array<code::Operand> *params, code::Var resultIn) {
		using namespace code;

		// Do we need a parameter for the result?
		bool resultParam = resultIn != Var();
		resultParam &= !result.returnInReg();

		nat start = 0;

		if (resultParam) {
			if (isMember()) {
				assert(params->count() >= 1);
				assert(params->at(0) != Operand(ptrA));
				addParam(to, params, 0);
				start = 1;
			}

			*to->to << lea(ptrA, ptrRel(resultIn, Offset()));
			*to->to << fnParam(ptrA);
		}

		for (nat i = start; i < params->count(); i++) {
			assert(!resultParam || params->at(i) != Operand(ptrA));
			addParam(to, params, i);
		}
	}

	void Function::addParam(CodeGen *to, Array<code::Operand> *params, nat id) {
		Value p = this->params->at(id);
		if (!p.ref && p.isValue()) {
			if (params->at(id).type() == code::opVariable) {
				*to->to << fnParam(params->at(id).var(), p.copyCtor());
			} else {
				// Has to be a reference to the value...
				*to->to << fnParamRef(params->at(id), p.type->size(), p.copyCtor());
			}
		} else {
			*to->to << fnParam(params->at(id));
		}
	}

	static inline code::Operand toOp(bool v) {
		return code::byteConst(v ? 1 : 0);
	}

	void Function::threadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *res, code::Operand thread) {
		using namespace code;

		Engine &e = engine();
		Block b = to->to->createBlock(to->to->last(to->block));
		*to->to << begin(b);
		CodeGen *sub = to->child(b);

		PrepareResult r = prepareThreadCall(sub, params);

		// Where shall we store the result (store the pointer to it in ptrB)?
		VarInfo resultPos;
		if (this->result == Value()) {
			// null-pointer.
			*to->to << mov(ptrB, ptrConst(Offset(0)));
		} else {
			resultPos = res->safeLocation(to, this->result);
			*to->to << lea(ptrB, resultPos.v);
		}

		Ref fn = ref();
		if (RefSource *t = threadThunk())
			fn = Ref(t);

		// Describe the return type.
		Var returnType = createBasicTypeInfo(to, this->result);

		// Spawn the thread!
		*to->to << lea(ptrA, r.params);
		*to->to << lea(ptrC, returnType);
		*to->to << fnParam(fn);
		*to->to << fnParam(toOp(isMember()));
		*to->to << fnParam(ptrA);
		*to->to << fnParam(ptrB);
		*to->to << fnParam(ptrC);
		*to->to << fnParam(thread);
		*to->to << fnParam(r.data);
		*to->to << fnCall(e.ref(Engine::rSpawnResult), valVoid());

		*to->to << end(b);
		resultPos.created(to);
	}

	Function::PrepareResult Function::prepareThreadCall(CodeGen *to, Array<code::Operand> *params) {
		using namespace code;

		Engine &e = engine();

		// Create a UThreadData object.
		Var data = to->to->createVar(to->block, Size::sPtr, e.ref(Engine::rAbortSpawn), freeOnException);
		*to->to << fnCall(e.ref(Engine::rSpawnLater), valPtr());
		*to->to << mov(data, ptrA);

		// Find out the pointer to the data and create FnParams object.
		*to->to << fnParam(ptrA);
		*to->to << fnCall(e.ref(Engine::rSpawnParam), valPtr());
		Var fnParams = createFnParams(to, code::Operand(ptrA));

		// Add all parameters.
		for (nat i = 0; i < params->count(); i++) {
			if (i == 0 && wcscmp(name->c_str(), Type::CTOR) == 0)
				addFnParam(to, fnParams, this->params->at(i), params->at(i));
			else
				addFnParamCopy(to, fnParams, this->params->at(i), params->at(i));
		}

		// Set the thread data to null, so that we do not double-free it if
		// the call returns with an exception.
		Var dataNoFree = to->to->createVar(to->block, Size::sPtr);
		*to->to << mov(dataNoFree, data);
		*to->to << mov(data, ptrConst(Offset()));

		PrepareResult r = { fnParams, dataNoFree };
		return r;
	}

	void Function::threadThunkParam(nat id, code::Listing *l) {
		using namespace code;

		Value t = params->at(id);
		if (id == 0 && wcscmp(name->c_str(), Type::CTOR) == 0) {
			Var v = l->createParam(valPtr());
			*l << fnParam(v);
		} else if (t.isHeapObj()) {
			Var v = l->createParam(valPtr());
			*l << fnParam(v);
		} else if (t.isBuiltIn()) {
			Var v = l->createParam(t.valType());
			*l << fnParam(v);
		} else {
			Var v = l->createParam(t.valType(), t.destructor(), freeOnBoth | freePtr);
			*l << fnParam(v, t.copyCtor());
		}
	}

	code::RefSource *Function::threadThunk() {
		using namespace code;

		if (threadThunkRef)
			return threadThunkRef;

		bool needsThunk = false;
		needsThunk |= !result.isBuiltIn();
		for (nat i = 0; i < params->count(); i++) {
			needsThunk |= params->at(i).isValue();
		}

		if (!needsThunk)
			return null;


		CodeGen *s = new (this) CodeGen(runOn());
		Listing *l = s->to;
		*l << prolog();

		nat firstParam = 0;

		if (isMember()) {
			// The this-ptr goes before any result parameter!
			threadThunkParam(0, l);
			firstParam = 1;
		}

		Var resultParam;
		if (result.isValue()) {
			resultParam = l->createParam(valPtr());
			*l << fnParam(resultParam);
		}

		for (nat i = firstParam; i < params->count(); i++)
			threadThunkParam(i, l);

		if (result.isHeapObj()) {
			Var t = l->createVar(l->root(), Size::sPtr);
			*l << fnCall(ref(), valPtr());
			*l << mov(t, ptrA);
			// Copy it...
			TODO(L"Implement clone here!");
			// *l << fnParam(ptrA);
			// *l << fnCall(stdCloneFn(result), retPtr());
		} else if (result.isValue()) {
			*l << fnCall(ref(), valPtr());

			CodeGen *sub = s->child(l->createBlock(l->root()));
			// Keep track of the result object.
			Var freeResult = l->createVar(sub->block, Size::sPtr, result.destructor(), freeOnException);
			*l << begin(sub->block);
			*l << mov(freeResult, resultParam);

			// We need to create a CloneEnv object.
			TODO(L"Implement clone here!");
			// Type *envType = CloneEnv::stormType(engine());
			// Var cloneEnv = l->createVar(l.frame.root(), Size::sPtr());
			// allocObject(sub, envType->defaultCtor(), Actuals(), cloneEnv);

			// // Find 'deepCopy'.
			// Function *deepCopy = result.type->deepCopyFn();
			// if (!deepCopy)
			// 	throw InternalError(L"The type " + ::toS(result) + L" does not have the required 'deepCopy' member.");

			// // Copy by calling 'deepCopy'.
			// *l << fnParam(resultParam);
			// *l << fnParam(cloneEnv);
			// *l << fnCall(deepCopy->ref(), retPtr());

			*l << end(sub->block);
			*l << mov(ptrA, resultParam);
		} else {
			*l << fnCall(ref(), result.valType());
		}

		*l << epilog();
		if (result.isBuiltIn())
			*l << ret(result.valType());
		else
			*l << ret(valPtr());

		threadThunkCode = new (this) Binary(engine().arena(), l);
		threadThunkRef = new (this) RefSource(*identifier() + new (this) Str(L"<thunk>"));
		threadThunkRef->set(threadThunkCode);
		return threadThunkRef;
	}


	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		asyncThreadCall(to, params, result, ptrConst(Offset()));
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand t) {
		using namespace code;

		Engine &e = engine();
		Block b = to->to->createBlock(to->to->last(to->block));
		*to->to << begin(b);

		CodeGen *sub = to->child(b);
		PrepareResult r = prepareThreadCall(sub, params);

		// // Create the result object.
		// Type *futureT = wrapFuture(e, this->result).type;
		// VarInfo resultPos = result->safeLocation(sub, thisPtr(futureT));
		// allocObject(sub, futureT->defaultCtor(), Actuals(), resultPos.var());
		// resultPos.created(sub);

		// // Find out what to call...
		// const RefSource *fn = threadThunk();
		// if (!fn)
		// 	fn = &ref();

		// // Return type...
		// Variable returnType = createBasicTypeInfo(to, this->result);

		// // Now we're ready to spawn the thread!
		// *to->to << lea(ptrA, r.params);
		// *to->to << lea(ptrC, returnType);
		// *to->to << fnParam(*fn);
		// *to->to << fnParam(toVal(isMember()));
		// *to->to << fnParam(ptrA);
		// *to->to << fnParam(resultPos.var());
		// *to->to << fnParam(ptrC);
		// *to->to << fnParam(t);
		// *to->to << fnParam(r.data);
		// *to->to << fnCall(e.fnRefs.spawnFuture, retVoid());

		*to->to << end(b);
		TODO(L"Implement me!");
		assert(false);
	}


	/**
	 * Low-level functions called by the generated code.
	 */

	void spawnThreadResult(const void *fn, bool member, const os::FnParams *params, void *result,
						BasicTypeInfo *resultType, Thread *on, os::UThreadData *data) {
		os::FutureSema<os::Sema> future;
		const os::Thread *thread = on ? &on->thread() : null;
		os::UThread::spawn(fn, member, *params, future, result, *resultType, thread, data);
		future.result();
	}

	void spawnThreadFuture(const void *fn, bool member, const os::FnParams *params, FutureBase *result,
						BasicTypeInfo *resultType, Thread *on, os::UThreadData *data) {
		os::FutureBase *future = result->rawFuture();
		const os::Thread *thread = on ? &on->thread() : null;
		os::UThread::spawn(fn, member, *params, *future, result->rawResult(), *resultType, thread, data);
	}



	/**
	 * Convenience functions.
	 */

	Function *inlinedFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<void, InlineParams> *fn) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) InlineCode(fn));
		return r;
	}

	Function *nativeFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) StaticCode(fn));
		return r;
	}

	Function *nativeEngineFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) StaticEngineCode(result, fn));
		return r;
	}

	Function *lazyFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<CodeGen *> *generate) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) LazyCode(generate));
		return r;
	}


}
