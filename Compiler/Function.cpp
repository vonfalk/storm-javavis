#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "Exception.h"
#include "Code.h"
#include "Core/Str.h"
#include "Lib/Future.h"
#include "Lib/Clone.h"

namespace storm {

	Function::Function(Value result, Str *name, Array<Value> *params) :
		Named(name, params),
		result(result),
		lookupRef(null),
		codeRef(null),
		runOnThread(null),
		isPure(false) {}

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
		RunOn result;
		if (Type *t = as<Type>(parent())) {
			result = t->runOn();
		}

		if (result.state != RunOn::any)
			return result;

		// The function itself can override if the parent class says "run on any thread".
		if (runOnThread) {
			return RunOn(runOnThread);
		} else {
			return RunOn(RunOn::any);
		}
	}

	void Function::runOn(NamedThread *t) {
		runOnThread = t;
	}

	Bool Function::pure() const {
		return isPure;
	}

	Function *Function::makePure() {
		isPure = true;
		return this;
	}

	Function *Function::makePure(Bool v) {
		isPure = v;
		return this;
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

	const void *Function::originalPtr() {
		return directRef().address();
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

		Var r = s->l->createVar(s->block, Size::sPtr);

		switch (on.state) {
		case RunOn::runtime:
			// Should be a this-ptr. Does not work well for constructors.
			assert(wcscmp(name->c_str(), Type::CTOR) != 0,
				L"Please overload 'findThread' for your constructor '" + ::toS(identifier()) + L"'!");
			*s->l << mov(ptrA, params->at(0));
			*s->l << add(ptrA, engine().ref(Engine::rTObjectOffset));
			*s->l << mov(r, ptrRel(ptrA, Offset()));
			break;
		case RunOn::named:
			*s->l << mov(r, on.thread->ref());
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
			throw InternalError(L"Parameter count mismatch when calling " + ::toS(identifier())
								+ L". Got " + ::toS(params->count()) + L" parameter.");

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

		addParams(to, params);

		if (result == Value()) {
			*to->l << fnCall(ref, isMember());
		} else {
			VarInfo rVar = res->safeLocation(to, result);
			*to->l << fnCall(ref, isMember(), result.desc(engine()), rVar.v);
			rVar.created(to);
		}
	}

	void Function::addParams(CodeGen *to, Array<code::Operand> *params) {
		for (nat i = 0; i < params->count(); i++) {
			addParam(to, params, i);
		}
	}

	void Function::addParam(CodeGen *to, Array<code::Operand> *params, nat id) {
		Value p = this->params->at(id);
		if (!p.ref && p.isValue()) {
			if (params->at(id).type() == code::opVariable) {
				*to->l << fnParam(p.desc(engine()), params->at(id).var());
			} else {
				// Has to be a reference to the value...
				*to->l << fnParamRef(p.desc(engine()), params->at(id));
			}
		} else {
			*to->l << fnParam(p.desc(engine()), params->at(id));
		}
	}

	static inline code::Operand toOp(bool v) {
		return code::byteConst(v ? 1 : 0);
	}

	void Function::threadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *res, code::Operand thread) {
		using namespace code;

		Engine &e = engine();
		CodeGen *sub = to->child();
		*to->l << begin(sub->block);

		// Create the parameters.
		Var par = createFnCall(sub, this->params, params, true);

		// Where shall we store the result (store the pointer to it in ptrB)?
		VarInfo resultPos;
		if (this->result == Value()) {
			// null-pointer.
			*to->l << mov(ptrB, ptrConst(Offset(0)));
		} else {
			resultPos = res->safeLocation(sub, this->result);
			*to->l << lea(ptrB, resultPos.v);
		}

		Ref thunk = Ref(threadThunk());

		TypeDesc *ptr = e.ptrDesc();

		// Spawn the thread!
		*to->l << lea(ptrA, par);
		*to->l << fnParam(ptr, ref()); // fn
		*to->l << fnParam(byteDesc(e), toOp(isMember())); // member
		*to->l << fnParam(ptr, thunk); // thunk
		*to->l << fnParam(ptr, ptrA); // params
		*to->l << fnParam(ptr, ptrB); // result
		*to->l << fnParam(ptr, thread); // on
		*to->l << fnCall(e.ref(Engine::rSpawnResult), false);
		resultPos.created(sub);

		*to->l << end(sub->block);

		// May be delayed...
		if (res->needed())
			res->location(to).created(to);

		// Clone the result.
		if (result.isValue() && !result.isBuiltIn()) {
			if (Function *f = result.type->deepCopyFn()) {
				CodeGen *child = to->child();
				*to->l << begin(child->block);

				Var env = allocObject(child, CloneEnv::stormType(e));
				*to->l << lea(ptrA, resultPos.v);
				*to->l << fnParam(ptr, ptrA);
				*to->l << fnParam(ptr, env);
				*to->l << fnCall(f->ref(), true);

				*to->l << end(child->block);
			}
		} else if (result.isClass()) {
			*to->l << fnParam(ptr, resultPos.v);
			*to->l << fnCall(cloneFn(result.type)->ref(), false, ptr, resultPos.v);
		}
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		asyncThreadCall(to, params, result, ptrConst(Offset()));
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread) {
		using namespace code;

		Engine &e = engine();
		CodeGen *sub = to->child();
		*to->l << begin(sub->block);

		// Create the parameters.
		Var par = createFnCall(sub, this->params, params, true);

		// Create the result object.
		Type *futureT = wrapFuture(e, this->result).type;
		VarInfo resultPos = result->safeLocation(sub, thisPtr(futureT));
		allocObject(sub, futureT->defaultCtor(), new (this) Array<Operand>(), resultPos.v);
		resultPos.created(sub);

		// Get the thunk.
		Ref thunk = Ref(threadThunk());

		TypeDesc *ptr = e.ptrDesc();

		// Spawn the thread!
		*to->l << lea(ptrA, par);
		*to->l << fnParam(ptr, ref()); // fn
		*to->l << fnParam(byteDesc(e), toOp(isMember())); // member
		*to->l << fnParam(ptr, thunk); // thunk
		*to->l << fnParam(ptr, ptrA); // params
		*to->l << fnParam(ptr, resultPos.v); // result
		*to->l << fnParam(ptr, thread); // on
		*to->l << fnCall(e.ref(Engine::rSpawnFuture), false);

		// Now, we're done!
		*to->l << end(sub->block);

		// May be delayed...
		if (result->needed())
			result->location(to).created(to);
	}

	code::RefSource *Function::threadThunk() {
		using namespace code;

		if (threadThunkRef)
			return threadThunkRef;

		Binary *threadThunkCode = callThunk(result, params);
		threadThunkRef = new (this) RefSource(*identifier() + new (this) Str(L"<thunk>"));
		threadThunkRef->set(threadThunkCode);
		return threadThunkRef;
	}


	/**
	 * Low-level functions called by the generated code.
	 */

	void spawnThreadResult(const void *fn, bool member, os::CallThunk thunk, void **params, void *result, Thread *on) {
		os::FnCallRaw call(params, thunk);
		os::FutureSema<os::Sema> future;
		const os::Thread *thread = on ? &on->thread() : null;
		os::UThread::spawnRaw(fn, member, null, call, future, result, thread);
		future.result();
	}

	void spawnThreadFuture(const void *fn, bool member, os::CallThunk thunk, void **params, FutureBase *result, Thread *on) {
		os::FnCallRaw call(params, thunk);
		const os::Thread *thread = on ? &on->thread() : null;
		os::UThread::spawnRaw(fn, member, null, call, *result->rawFuture(), result->rawResult(), thread);
	}

	/**
	 * CppFunction.
	 */

	CppMemberFunction::CppMemberFunction(Value result, Str *name, Array<Value> *params, const void *original) :
		Function(result, name, params), original(original) {}

	const void *CppMemberFunction::originalPtr() {
		return original;
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

	Function *dynamicFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, code::Listing *src) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) DynamicCode(src));
		return r;
	}

}
