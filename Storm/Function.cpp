#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "TypeDtor.h"
#include "Exception.h"
#include "Lib/CloneEnv.h"
#include "Lib/TObject.h"
#include "Lib/Future.h"
#include "Code/Instruction.h"
#include "Code/VTable.h"
#include "CodeGen.h"

namespace storm {

	void validateParams(const vector<Value> &params) {
		for (nat i = 0; i < params.size(); i++)
			assert(params[i] != Value(), L"void parameters will get strange errors, beware!");
	}

	Function::Function(Value result, const String &name, const vector<Value> &params)
		: Named(name, params), result(result), lookupRef(null), codeRef(null) {
		validateParams(params);
	}

	Function::Function(Value result, Par<Str> name, Par<Array<Value>> params)
		: Named(name, params), result(result), lookupRef(null), codeRef(null) {
		validateParams(this->params);
	}

	Function::~Function() {
		// Correct destruction order.
		lookup = null;
		code = null;
		engine().destroy(threadThunkCode);
		delete threadThunkRef;
		delete lookupRef;
		delete codeRef;
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

	void Function::runOn(Par<NamedThread> thread) {
		runOnThread = thread;
	}

	void *Function::pointer() {
		return ref().address();
	}

	bool Function::isMember() {
		return as<Type>(parent()) != null;
	}

	code::RefSource &Function::ref() {
		initRefs();
		return *lookupRef;
	}

	code::RefSource &Function::directRef() {
		initRefs();
		return *codeRef;
	}

	code::Variable Function::findThread(Par<CodeGen> s, const Actuals &params) {
		using namespace code;

		RunOn on = runOn();
		assert(on.state != RunOn::any, L"Only use 'findThread' on functions which 'runOn()' something other than any.");

		Variable r = s->frame.createPtrVar(s->block.v);

		switch (on.state) {
		case RunOn::runtime:
			// Should be a this-ptr. Does not work well for constructors.
			assert(name != Type::CTOR, L"Please specify for your constructor semantics!");
			s->to << mov(ptrA, params[0]);
			s->to << mov(r, ptrRel(ptrA, TObject::threadOffset()));
			break;
		case RunOn::named:
			s->to << mov(r, on.thread->ref());
			break;
		default:
			assert(false, L"Unknown state.");
			break;
		}

		return r;
	}

	void Function::autoCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> result) {
		RunOn r = runOn();
		if (to->runOn.canRun(r)) {
			localCall(to, params, result, true);
		} else {
			code::Variable v = findThread(to, params);
			threadCall(to, params, result, v);
		}
	}

	void Function::localCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> res, bool useLookup) {
		initRefs();
		assert(params.size() == this->params.size());

		InlinedCode *inlined = as<InlinedCode>(code.borrow());
		// If we're not going to use the lookup, we may choose to inline sooner.
		if (useLookup && as<DelegatedCode>(lookup.borrow()) == null)
			inlined = null;

		if (inlined)
			inlined->code(to, params, res);
		else
			localCall(to, params, res, useLookup ? this->ref() : directRef());
	}

	void Function::addParam(Par<CodeGen> to, const Actuals &params, nat id) {
		const Value &p = this->params[id];
		if (!p.ref && p.isValue()) {
			to->to << fnParam(params[id].variable(), p.copyCtor());
		} else {
			to->to << fnParam(params[id]);
		}
	}

	void Function::addParams(Par<CodeGen> to, const Actuals &params, const code::Variable &resultIn) {
		using namespace code;

		// Do we need a parameter for the result?
		bool resultParam = resultIn != Variable::invalid;
		resultParam &= !result.returnInReg();

		nat start = 0;

		if (resultParam) {
			if (isMember()) {
				assert(params.size() >= 1);
				assert(params[0] != code::Value(ptrA));
				addParam(to, params, 0);
				start = 1;
			}

			to->to << lea(ptrA, ptrRel(resultIn));
			to->to << fnParam(ptrA);
		}

		for (nat i = start; i < params.size(); i++) {
			assert(!resultParam || params[i] != code::Value(ptrA));
			addParam(to, params, i);
		}
	}

	void Function::localCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> res, code::Ref ref) {
		using namespace code;

		if (result == Value()) {
			addParams(to, params, code::Variable());

			to->to << fnCall(ref, Size());
		} else {
			VarInfo result = res->safeLocation(to, this->result);
			addParams(to, params, result.var());

			if (this->result.returnInReg()) {
				to->to << fnCall(ref, result.var().size());
				to->to << mov(result.var(), asSize(ptrA, result.var().size()));
			} else {
				// Ignore return value...
				to->to << fnCall(ref, Size());
			}

			result.created(to);
		}
	}

	void spawnThreadResult(const void *fn, bool member, const code::FnParams *params, void *result,
						BasicTypeInfo *resultType, Thread *on, code::UThreadData *data) {
		code::FutureSema<code::Sema> future(result);
		code::UThread::spawn(fn, member, *params, future, *resultType, &on->thread, data);
		future.result();
	}

	void spawnThreadFuture(const void *fn, bool member, const code::FnParams *params, FutureBase *result,
								BasicTypeInfo *resultType, Thread *on, code::UThreadData *data) {
		code::FutureBase *future = result->rawFuture();
		code::UThread::spawn(fn, member, *params, *future, *resultType, &on->thread, data);
	}

	Function::PrepareResult Function::prepareThreadCall(Par<CodeGen> to, const Actuals &params) {
		using namespace code;

		Engine &e = engine();

		// Create a UThreadData object.
		Variable data = to->frame.createPtrVar(to->block.v, e.fnRefs.abortSpawn, freeOnException);
		to->to << fnCall(e.fnRefs.spawnLater, Size::sPtr);
		to->to << mov(data, ptrA);

		// Find out the pointer to the data and create FnParams object.
		to->to << fnParam(ptrA);
		to->to << fnCall(e.fnRefs.spawnParam, Size::sPtr);
		Variable fnParams = createFnParams(to, code::Value(ptrA)).v;

		// Add all parameters.
		for (nat i = 0; i < params.size(); i++) {
			if (i == 0 && name == Type::CTOR)
				addFnParam(to, fnParams, this->params[i], params[i], false);
			else
				addFnParamCopy(to, fnParams, this->params[i], params[i], true);
		}

		// Set the thread data to null, so that we do not double-free it if
		// the call returns with an exception.
		Variable dataNoFree = to->frame.createPtrVar(to->block.v);
		to->to << mov(dataNoFree, data);
		to->to << mov(data, intPtrConst(0));

		PrepareResult r = { fnParams, dataNoFree };
		return r;
	}

	static inline code::Value toVal(bool v) {
		return code::byteConst(v ? 1 : 0);
	}

	void Function::threadCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> res, const code::Value &thread) {
		using namespace code;

		Engine &e = engine();
		Block b = to->frame.createChild(to->frame.last(to->block.v));
		to->to << begin(b);
		Auto<CodeGen> sub = to->child(b);

		PrepareResult r = prepareThreadCall(sub, params);

		// Where shall we store the result (store the pointer to it in ptrB);
		VarInfo resultPos;
		if (this->result == Value()) {
			// null-pointer.
			to->to << mov(ptrB, intPtrConst(0));
		} else {
			resultPos = res->safeLocation(to, this->result);
			to->to << lea(ptrB, resultPos.var());
		}

		const RefSource *fn = threadThunk();
		if (!fn)
			fn = &ref();

		// Describe the return type.
		Variable returnType = createBasicTypeInfo(to, this->result);

		// Spawn the thread!
		to->to << lea(ptrA, r.params);
		to->to << lea(ptrC, returnType);
		to->to << fnParam(*fn);
		to->to << fnParam(toVal(isMember()));
		to->to << fnParam(ptrA);
		to->to << fnParam(ptrB);
		to->to << fnParam(ptrC);
		to->to << fnParam(thread);
		to->to << fnParam(r.data);
		to->to << fnCall(e.fnRefs.spawnResult, Size());

		to->to << end(b);
		resultPos.created(to);
	}

	void Function::asyncThreadCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> result, const code::Value &t) {
		using namespace code;

		Engine &e = engine();
		Block b = to->frame.createChild(to->frame.last(to->block.v));
		to->to << begin(b);

		Auto<CodeGen> sub = to->child(b);
		PrepareResult r = prepareThreadCall(sub, params);

		// Create the result object.
		Type *futureT = futureType(e, this->result);
		VarInfo resultPos = result->safeLocation(sub, Value::thisPtr(futureT));
		allocObject(sub, futureT->defaultCtor(), Actuals(), resultPos.var());
		resultPos.created(sub);

		// Find out what to call...
		const RefSource *fn = threadThunk();
		if (!fn)
			fn = &ref();

		// Return type...
		Variable returnType = createBasicTypeInfo(to, this->result);

		// Now we're ready to spawn the thread!
		to->to << lea(ptrA, r.params);
		to->to << lea(ptrC, returnType);
		to->to << fnParam(*fn);
		to->to << fnParam(toVal(isMember()));
		to->to << fnParam(ptrA);
		to->to << fnParam(resultPos.var());
		to->to << fnParam(ptrC);
		to->to << fnParam(t);
		to->to << fnParam(r.data);
		to->to << fnCall(e.fnRefs.spawnFuture, Size());

		to->to << end(b);
	}

	code::RefSource *Function::threadThunk() {
		using namespace code;

		if (threadThunkRef)
			return threadThunkRef;

		bool needsThunk = false;
		needsThunk |= !result.isBuiltIn();
		for (nat i = 0; i < params.size(); i++) {
			needsThunk |= !params[i].isBuiltIn();
		}

		if (!needsThunk)
			return null;


		Auto<CodeGen> s = CREATE(CodeGen, this, RunOn());
		Listing &l = s->l->v;
		l << prolog();

		Variable resultParam;
		if (result.isValue()) {
			resultParam = l.frame.createPtrParam();
			l << fnParam(resultParam);
		}

		for (nat i = 0; i < params.size(); i++) {
			const Value &t = params[i];
			if (i == 0 && name == Type::CTOR) {
				Variable v = l.frame.createPtrParam();
				l << fnParam(v);
			} else if (t.isClass()) {
				Variable v = l.frame.createPtrParam(engine().fnRefs.release);
				l << fnParam(v);
			} else if (t.isBuiltIn()) {
				Variable v = l.frame.createParameter(t.size(), false);
				l << fnParam(v);
			} else {
				Variable v = l.frame.createParameter(t.size(), false, t.destructor(), freeOnBoth | freePtr);
				l << fnParam(v, t.copyCtor());
			}
		}

		if (result.isClass()) {
			Variable t = l.frame.createPtrVar(l.frame.root(), engine().fnRefs.release);
			l << fnCall(ref(), Size::sPtr);
			l << mov(t, ptrA);
			// Copy it...
			l << fnParam(ptrA);
			l << fnCall(stdCloneFn(result).v, Size::sPtr);
		} else if (result.isValue()) {
			l << fnCall(ref(), Size::sPtr);

			Auto<CodeGen> sub = s->child(l.frame.createChild(l.frame.root()));
			// Keep track of the result object.
			Variable freeResult = l.frame.createPtrVar(sub->block.v, result.destructor(), freeOnException);
			l << begin(sub->block.v);
			l << mov(freeResult, resultParam);

			// We need to create a CloneEnv object.
			Type *envType = CloneEnv::stormType(engine());
			Variable cloneEnv = l.frame.createPtrVar(l.frame.root(), engine().fnRefs.release);
			allocObject(sub, envType->defaultCtor(), Actuals(), cloneEnv);

			// Find 'deepCopy'.
			Function *deepCopy = result.type->deepCopyFn();
			if (!deepCopy)
				throw InternalError(L"The type " + ::toS(result) + L" does not have the required 'deepCopy' member.");

			// Copy by calling 'deepCopy'.
			l << fnParam(resultParam);
			l << fnParam(cloneEnv);
			l << fnCall(deepCopy->ref(), Size::sPtr);

			l << end(sub->block.v);
			l << mov(ptrA, resultParam);
		} else {
			l << fnCall(ref(), result.size());
		}

		l << epilog();
		if (result.isBuiltIn())
			l << ret(result.size());
		else
			l << ret(Size::sPtr);

		threadThunkCode = new Binary(engine().arena, l);
		threadThunkRef = new RefSource(engine().arena, identifier() + L"<thunk>");
		threadThunkRef->set(threadThunkCode);
		return threadThunkRef;
	}

	void Function::setCode(Par<Code> code) {
		if (this->code)
			this->code->detach();
		code->attach(this);
		this->code = code;
		if (codeRef)
			code->update(*codeRef);
	}

	void Function::setLookup(Par<Code> code) {
		if (lookup)
			lookup->detach();

		if (code == null && codeRef != null)
			lookup = CREATE(DelegatedCode, engine(), *codeRef);
		else
			lookup = code;

		if (lookup) {
			lookup->attach(this);

			if (lookupRef)
				lookup->update(*lookupRef);
		}
	}

	void Function::initRefs() {
		if (!codeRef) {
			assert(parent(), "Too early!");
			codeRef = new code::RefSource(engine().arena, identifier() + L"<c>");
			if (code)
				code->update(*codeRef);
		}

		if (!lookupRef) {
			assert(parent(), "Too early!");
			lookupRef = new code::RefSource(engine().arena, identifier() + L"<l>");
			if (!lookup) {
				lookup = CREATE(DelegatedCode, engine(), *codeRef);
				lookup->attach(this);
			}
			lookup->update(*lookupRef);
		}
	}

	void Function::output(wostream &to) const {
		to << result << " " << name << "(";
		join(to, params, L", ");
		to << ")";
	}

	NativeFunction::NativeFunction(Value result, const String &name, const vector<Value> &params, nat slot) :
		Function(result, name, params), vtableSlot(slot) {}

	bool isOverload(Function *base, Function *overload) {
		if (base->name != overload->name)
			return false;

		if (base->params.size() != overload->params.size())
			return false;

		if (base->params.size() <= 0)
			return false;

		// First parameter is special.
		if (!base->params[0].canStore(overload->params[0]))
			return false;

		for (nat i = 1; i < base->params.size(); i++)
			if (!overload->params[i].canStore(overload->params[i]))
				return false;

		return true;
	}

	Function *nativeFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr) {
		Function *fn = CREATE(Function, e, result, name, params);
		Auto<StaticCode> c = CREATE(StaticCode, e, ptr);
		fn->setCode(c);
		return fn;
	}

	Function *nativeEngineFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr) {
		Function *fn = CREATE(Function, e, result, name, params);
		Auto<StaticEngineCode> c = CREATE(StaticEngineCode, e, result, ptr);
		fn->setCode(c);
		return fn;
	}

	Function *nativeMemberFunction(Engine &e, Type *member, Value result,
								const String &name, const vector<Value> &params,
								void *ptr) {
		void *vtable = member->vtable.baseVTable();
		void *plain = null;
		Function *fn = null;

		if (member->flags & typeClass) {
			plain = code::deVirtualize(ptr, vtable);

			// Note: the findSlot here is predictable even after optimizations since we can rely on
			// reading the code of 'ptr' to see which entry it uses instead of looking through the table
			// for potentially similar items.
			nat slot = code::findSlot(ptr, vtable);
			fn = CREATE(NativeFunction, e, result, name, params, slot);
		} else {
			fn = CREATE(Function, e, result, name, params);
		}

		if (plain) {
			fn->setCode(steal(CREATE(StaticCode, e, plain)));
			fn->setLookup(steal(CREATE(StaticCode, e, ptr)));
		} else {
			fn->setCode(steal(CREATE(StaticCode, e, ptr)));
		}

		return fn;
	}

	Function *nativeDtor(Engine &e, Type *member, void *ptr) {
		Function *fn = CREATE(Function, e, Value(), Type::DTOR, vector<Value>(1, Value::thisPtr(member)));

		// For destructors, we get the lookup directly.
		// We have to find the raw function ourselves.
		void *vtable = member->vtable.baseVTable();

		if (vtable) {
			// We need to wrap the raw pointer since dtors generally does not use the same calling convention
			// as regular functions.
			void *raw = code::vtableDtor(vtable);
			fn->setLookup(steal(CREATE(StaticCode, e, ptr)));
			fn->setCode(steal(wrapRawDestructor(e, raw)));
		} else {
			fn->setCode(steal(CREATE(StaticCode, e, ptr)));
		}

		return fn;
	}


	Function *inlinedFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, Fn<void, InlinedParams> fn) {
		Function *f = CREATE(Function, e, result, name, params);
		Auto<InlinedCode> ic = CREATE(InlinedCode, e, fn);
		f->setCode(ic);
		return f;
	}

	Function *dynamicFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, const code::Listing &l) {
		Auto<Function> f = CREATE(Function, e, result, name, params);
		Auto<DynamicCode> dc = CREATE(DynamicCode, e, l);
		f->setCode(dc);
		return f.ret();
	}

	Function *lazyFunction(Engine &e, Value result, const String &name,
						const vector<Value> &params, Par<FnPtr<CodeGen *>> generate) {
		Auto<Function> f = CREATE(Function, e, result, name, params);
		Auto<LazyCode> c = CREATE(LazyCode, e, generate);
		f->setCode(c);
		return f.ret();
	}

}
