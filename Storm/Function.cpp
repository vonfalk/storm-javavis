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

namespace storm {

	Function::Function(Value result, const String &name, const vector<Value> &params)
		: Named(name, params), result(result), lookupRef(null), codeRef(null) {}

	Function::~Function() {
		// Correct destruction order.
		code = null;
		lookup = null;
		delete codeRef;
		delete lookupRef;
		engine().destroy(threadThunkCode);
		delete threadThunkRef;
	}

	RunOn Function::runOn() const {
		if (Type *t = as<Type>(parent())) {
			return t->runOn();
		}
		return RunOn(RunOn::any);
	}

	void *Function::pointer() {
		return ref().address();
	}

	code::RefSource &Function::ref() {
		initRefs();
		return *lookupRef;
	}

	code::RefSource &Function::directRef() {
		initRefs();
		return *codeRef;
	}

	code::Variable Function::findThread(const GenState &s, const Actuals &params) {
		using namespace code;

		RunOn on = runOn();
		assert(on.state != RunOn::any, L"Only use 'findThread' on functions which 'runOn()' something other than any.");

		Variable r = s.frame.createPtrVar(s.block);

		switch (on.state) {
		case RunOn::runtime:
			// Should be a this-ptr. Does not work well for constructors.
			assert(name != Type::CTOR, L"Please specify for your constructor semantics!");
			s.to << mov(ptrA, params[0]);
			s.to << mov(r, ptrRel(ptrA, TObject::threadOffset()));
			break;
		case RunOn::named:
			s.to << mov(r, on.thread->ref());
			break;
		default:
			assert(false, L"Unknown state.");
			break;
		}

		return r;
	}

	void Function::localCall(const GenState &to, const Actuals &params, GenResult &res, bool useLookup) {
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

	void Function::localCall(const GenState &to, const Actuals &params, GenResult &res, code::Ref ref) {
		using namespace code;

		if (result == Value()) {
			for (nat i = 0; i < params.size(); i++)
				to.to << fnParam(params[i]);

			to.to << fnCall(ref, Size());
		} else {
			VarInfo result = res.safeLocation(to, this->result);

			if (!this->result.returnOnStack()) {
				to.to << lea(ptrA, ptrRel(result.var));
				to.to << fnParam(ptrA);
			}

			for (nat i = 0; i < params.size(); i++) {
				if (this->params[i].isValue()) {
					assert(params[i].type() == code::Value::tVariable);
					to.to << fnParam(params[i].variable(), this->params[i].copyCtor());
				} else {
					to.to << fnParam(params[i]);
				}
			}

			if (this->result.returnOnStack()) {
				to.to << fnCall(ref, result.var.size());
				to.to << mov(result.var, asSize(ptrA, result.var.size()));
			} else {
				// Ignore return value...
				to.to << fnCall(ref, Size());
			}

			result.created(to);
		}
	}

	void spawnThread(const void *fn, const code::FnParams *params, Thread *on, code::UThreadData *data) {
		code::UThread::spawn(fn, *params, &on->thread, data);
	}

	void spawnThreadResult(const void *fn, const code::FnParams *params, void *result,
						BasicTypeInfo *resultType, Thread *on, code::UThreadData *data) {
		code::FutureSema<code::Sema> future(result);
		code::UThread::spawn(fn, *params, future, *resultType, &on->thread, data);
		future.result();
	}

	void spawnThreadFuture(const void *fn, const code::FnParams *params, FutureBase *result,
								BasicTypeInfo *resultType, Thread *on, code::UThreadData *data) {
		code::FutureBase *future = result->rawFuture();
		code::UThread::spawn(fn, *params, *future, *resultType, &on->thread, data);
	}

	// Find 'std:clone' for the given type.
	static code::Value stdCloneFn(Engine &e, const Value &type) {
		Auto<Name> name = CREATE(Name, e);
		name->add(L"core");
		name->add(L"clone", valList(1, type));
		Function *f = as<Function>(e.scope()->find(name));
		if (!f)
			throw InternalError(L"Could not find std.clone(" + ::toS(type) + L").");
		return code::Value(f->ref());
	}

	// Add a single parameter of type 'v', value in 'a' to 'to'.
	static void addParam(Engine &e, const GenState &z, code::Variable to, const Value &v, const code::Value &a) {
		using namespace code;

		if (v.isClass()) {
			// Deep copy the object.
			Variable clone = z.frame.createPtrVar(z.block, e.fnRefs.release);
			z.to << fnParam(a);
			z.to << fnCall(stdCloneFn(e, v), Size::sPtr);
			z.to << mov(clone, ptrA);

			z.to << lea(ptrC, to);
			z.to << lea(ptrA, clone);
			z.to << fnParam(ptrC);
			z.to << fnParam(e.fnRefs.copyRefPtr);
			z.to << fnParam(e.fnRefs.releasePtr);
			z.to << fnParam(natConst(v.size()));
			z.to << fnParam(ptrA);
			z.to << fnCall(e.fnRefs.fnParamsAdd, Size());
		} else if (v.ref || v.isBuiltIn()) {
			// No need for copy ctors!
			if (v.ref) {
				TODO(L"Should references be allowed in these function calls?");
			}
			z.to << lea(ptrC, to);
			z.to << fnParam(ptrC);
			z.to << fnParam(intPtrConst(0));
			z.to << fnParam(intPtrConst(0));
			z.to << fnParam(natConst(v.size()));
			z.to << fnParam(a);
			z.to << fnCall(e.fnRefs.fnParamsAdd, Size());
		} else {
			// It is a value, use the stdClone.
			code::Value dtor = v.destructor();
			if (dtor.empty())
				dtor = intPtrConst(0);
			z.to << lea(ptrC, to);
			z.to << lea(ptrA, a);
			z.to << fnParam(ptrC);
			z.to << fnParam(stdCloneFn(e, v));
			z.to << fnParam(dtor);
			z.to << fnParam(natConst(v.size()));
			z.to << fnParam(ptrA);
			z.to << fnCall(e.fnRefs.fnParamsAdd, Size());
		}
	}

	// Add the this-parameter of the first parameter.
	static void addCtorThis(Engine &e, const GenState &z, code::Variable to, const Value &v, const code::Value &a) {
		using namespace code;

		assert(v.size() == Size::sPtr);

		z.to << lea(ptrC, to);
		z.to << fnParam(ptrC);
		z.to << fnParam(intPtrConst(0));
		z.to << fnParam(intPtrConst(0));
		z.to << fnParam(natConst(Size::sPtr));
		z.to << fnParam(a);
		z.to << fnCall(e.fnRefs.fnParamsAdd, Size());
	}

	Function::PrepareResult Function::prepareThreadCall(const GenState &to, const Actuals &params) {
		using namespace code;

		Engine &e = engine();

		// Create a UThreadData object.
		Variable data = to.frame.createPtrVar(to.block, e.fnRefs.abortSpawn, freeOnException);
		to.to << fnCall(e.fnRefs.spawnLater, Size::sPtr);
		to.to << mov(data, ptrA);

		// Find out the pointer to the data and create FnParams object.
		to.to << fnParam(ptrA);
		to.to << fnCall(e.fnRefs.spawnParam, Size::sPtr);
		Variable fnParams = to.frame.createVariable(to.block,
													FnParams::classSize(),
													e.fnRefs.fnParamsDtor,
													freeOnBoth | freePtr);
		// Call the constructor of FnParams
		to.to << lea(ptrC, fnParams);
		to.to << fnParam(ptrC);
		to.to << fnParam(ptrA);
		to.to << fnCall(e.fnRefs.fnParamsCtor, Size());

		// Add all parameters.
		for (nat i = 0; i < params.size(); i++) {
			if (i == 0 && name == Type::CTOR)
				addCtorThis(e, to, fnParams, this->params[i], params[i]);
			else
				addParam(e, to, fnParams, this->params[i], params[i]);
		}

		// Set the thread data to null, so that we do not double-free it if
		// the call returns with an exception.
		Variable dataNoFree = to.frame.createPtrVar(to.block);
		to.to << mov(dataNoFree, data);
		to.to << mov(data, intPtrConst(0));

		PrepareResult r = { fnParams, dataNoFree };
		return r;
	}

	void Function::threadCall(const GenState &to, const Actuals &params, GenResult &res, const code::Value &thread) {
		using namespace code;

		Engine &e = engine();
		Block b = to.frame.createChild(to.frame.last(to.block));
		to.to << begin(b);
		GenState sub = to.child(b);

		PrepareResult r = prepareThreadCall(sub, params);

		// Where shall we store the result (store the pointer to it in ptrB);
		VarInfo resultPos;
		if (this->result == Value()) {
			// null-pointer.
			to.to << mov(ptrB, intPtrConst(0));
		} else {
			resultPos = res.safeLocation(to, this->result);
			to.to << lea(ptrB, resultPos.var);
		}

		const RefSource *fn = threadThunk();
		if (!fn)
			fn = &ref();

		// Describe the return type.
		Variable returnType = createBasicTypeInfo(to, this->result);

		// Spawn the thread!
		to.to << lea(ptrA, r.params);
		to.to << lea(ptrC, returnType);
		to.to << fnParam(*fn);
		to.to << fnParam(ptrA);
		to.to << fnParam(ptrB);
		to.to << fnParam(ptrC);
		to.to << fnParam(thread);
		to.to << fnParam(r.data);
		to.to << fnCall(e.fnRefs.spawnResult, Size());

		to.to << end(b);
		resultPos.created(to);
	}

	void Function::asyncThreadCall(const GenState &to, const Actuals &params, GenResult &result, const code::Value &t) {
		using namespace code;

		Engine &e = engine();
		Block b = to.frame.createChild(to.frame.last(to.block));
		to.to << begin(b);

		GenState sub = to.child(b);
		PrepareResult r = prepareThreadCall(sub, params);

		// Create the result object.
		Type *futureT = futureType(e, this->result);
		VarInfo resultPos = result.safeLocation(sub, Value::thisPtr(futureT));
		allocObject(sub, futureT->defaultCtor(), Actuals(), resultPos.var);
		resultPos.created(sub);

		// Find out what to call...
		const RefSource *fn = threadThunk();
		if (!fn)
			fn = &ref();

		// Return type...
		Variable returnType = createBasicTypeInfo(to, this->result);

		// Now we're ready to spawn the thread!
		to.to << lea(ptrA, r.params);
		to.to << lea(ptrC, returnType);
		to.to << fnParam(*fn);
		to.to << fnParam(ptrA);
		to.to << fnParam(resultPos.var);
		to.to << fnParam(ptrC);
		to.to << fnParam(t);
		to.to << fnParam(r.data);
		to.to << fnCall(e.fnRefs.spawnFuture, Size());

		to.to << end(b);
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


		Listing l;
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
			l << fnCall(stdCloneFn(engine(), result), Size::sPtr);
		} else if (result.isValue()) {
			l << fnCall(ref(), Size::sPtr);

			Block subBlock = l.frame.createChild(l.frame.root());
			// Keep track of the result object.
			Variable freeResult = l.frame.createPtrVar(subBlock, result.destructor(), freeOnException);
			l << begin(subBlock);
			l << mov(freeResult, resultParam);

			// We need to create a CloneEnv object.
			Type *envType = CloneEnv::type(engine());
			Variable cloneEnv = l.frame.createPtrVar(l.frame.root(), engine().fnRefs.release);
			allocObject(l, subBlock, envType->defaultCtor(), Actuals(), cloneEnv);

			// Find 'deepCopy'.
			Function *deepCopy = as<Function>(result.type->find(L"deepCopy", valList(2, result, Value::thisPtr(envType))));
			if (!deepCopy)
				throw InternalError(L"The type " + ::toS(result) + L" does not have the required 'deepCopy' member.");

			// Copy by calling 'deepCopy'.
			l << fnParam(resultParam);
			l << fnParam(cloneEnv);
			l << fnCall(deepCopy->ref(), Size::sPtr);

			l << end(subBlock);
			l << mov(ptrA, resultParam);
		} else {
			l << fnCall(ref(), result.size());
		}

		l << epilog();
		if (result.isBuiltIn())
			l << ret(result.size());
		else
			l << ret(Size::sPtr);

		String id = identifier() + L"<thunk>";
		threadThunkCode = new Binary(engine().arena, id, l);
		threadThunkRef = new RefSource(engine().arena, id);
		threadThunkCode->update(*threadThunkRef);
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
			lookup = CREATE(DelegatedCode, engine(), *codeRef, lookupRef->getTitle());
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
				lookup = CREATE(DelegatedCode, engine(), *codeRef, lookupRef->getTitle());
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

	Function *nativeMemberFunction(Engine &e, Type *member, Value result,
								const String &name, const vector<Value> &params,
								void *ptr) {
		void *vtable = member->vtable.baseVTable();
		void *plain = code::deVirtualize(ptr, vtable);

		Function *fn = CREATE(Function, e, result, name, params);
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

}
