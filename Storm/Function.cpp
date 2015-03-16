#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "TypeDtor.h"
#include "Exception.h"
#include "Lib/CloneEnv.h"
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
			TODO(L"Not implemented correctly for types yet!");
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

	void Function::genCode(const GenState &to, const Actuals &params, GenResult &res,
						const code::Value &thread, bool useLookup) {
		initRefs();
		assert(params.size() == this->params.size());
		code::Ref ref(useLookup ? this->ref() : directRef());

		if (thread.empty()) {
			bool inlined = true;
			// If we're not going to use the lookup, we may be more eager to inline!
			if (useLookup)
				inlined &= as<DelegatedCode>(lookup.borrow()) != 0;
			inlined &= as<InlinedCode>(code.borrow()) != 0;
			if (inlined)
				genCodeInline(to, params, res);
			else
				genCodeDirect(to, params, res, ref);
		} else {
			assert(useLookup, L"Non-lookup thunks not yet supported.");
			if (code::RefSource *r = threadThunk())
				ref = code::Ref(*r);
			genCodePost(to, params, res, ref, thread);
		}
	}

	void Function::genCodeInline(const GenState &to, const Actuals &params, GenResult &res) {
		InlinedCode *c = as<InlinedCode>(code.borrow());
		c->code(to, params, res);
	}

	void Function::genCodeDirect(const GenState &to, const Actuals &params, GenResult &res, code::Ref ref) {
		using namespace code;

		if (result == Value()) {
			for (nat i = 0; i < params.size(); i++)
				to.to << fnParam(params[i]);

			to.to << fnCall(ref, Size());
		} else {
			Variable result = res.safeLocation(to, this->result);

			if (!this->result.returnOnStack()) {
				to.to << lea(ptrA, ptrRel(result));
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
				to.to << fnCall(ref, result.size());
				to.to << mov(result, asSize(ptrA, result.size()));
			} else {
				// Ignore return value...
				to.to << fnCall(ref, Size());
			}
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

	// Find 'std:clone' for the given type.
	static code::Value stdCloneFn(Engine &e, const Value &type) {
		Auto<Name> name = CREATE(Name, e);
		name->add(L"core");
		name->add(L"clone", valList(1, type));
		Function *f = as<Function>(e.scope()->find(name));
		if (!f)
			throw InternalError(L"Could not find std.clone(" + ::toS(type) + L").");
		return code::Value(code::Ref(f->ref()));
	}

	// Add a single parameter of type 'v', value in 'a' to 'to'.
	static void addParam(Engine &e, const GenState &z, code::Variable to, const Value &v, const code::Value &a) {
		using namespace code;

		if (v.isClass()) {
			// Deep copy the object.
			Variable clone = z.frame.createPtrVar(z.block, Ref(e.fnRefs.release));
			z.to << fnParam(a);
			z.to << fnCall(stdCloneFn(e, v), Size::sPtr);
			z.to << mov(clone, ptrA);

			z.to << lea(ptrC, to);
			z.to << lea(ptrA, clone);
			z.to << fnParam(ptrC);
			z.to << fnParam(Ref(e.fnRefs.copyRefPtr));
			z.to << fnParam(Ref(e.fnRefs.releasePtr));
			z.to << fnParam(natConst(v.size()));
			z.to << fnParam(ptrA);
			z.to << fnCall(Ref(e.fnRefs.fnParamsAdd), Size());
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
			z.to << fnCall(Ref(e.fnRefs.fnParamsAdd), Size());
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
			z.to << fnCall(Ref(e.fnRefs.fnParamsAdd), Size());
		}
	}

void Function::genCodePost(const GenState &to, const Actuals &params, GenResult &res,
							code::Ref ref, const code::Value &thread) {
		using namespace code;

		Engine &e = engine();
		Block b = to.frame.createChild(to.block);
		to.to << begin(b);

		// Create a UThreadData object.
		Variable data = to.frame.createPtrVar(b, Ref(e.fnRefs.abortSpawn), freeOnException);
		to.to << fnCall(Ref(e.fnRefs.spawnLater), Size::sPtr);
		to.to << mov(data, ptrA);

		// Find out the pointer to the data and create FnParams object.
		to.to << fnParam(ptrA);
		to.to << fnCall(Ref(e.fnRefs.spawnParam), Size::sPtr);
		Variable fnParams = to.frame.createVariable(b,
													FnParams::classSize(),
													Ref(e.fnRefs.fnParamsDtor),
													freeOnBoth | freePtr);
		// Call the constructor of FnParams
		to.to << lea(ptrC, fnParams);
		to.to << fnParam(ptrC);
		to.to << fnParam(ptrA);
		to.to << fnCall(Ref(e.fnRefs.fnParamsCtor), Size());

		// Add all parameters.
		for (nat i = 0; i < params.size(); i++)
			addParam(e, to.child(b), fnParams, this->params[i], params[i]);

		// Describe the return type.
		Variable returnType = createBasicTypeInfo(to.child(b), this->result);

		// Set the thread data to null, so that we do not double-free it if
		// the call returns with an exception.
		Variable dataNoFree = to.frame.createPtrVar(b);
		to.to << mov(dataNoFree, data);
		to.to << mov(data, intPtrConst(0));

		// Where shall we store the result (store the pointer to it in ptrB);
		if (this->result == Value()) {
			// null-pointer.
			to.to << mov(ptrB, intPtrConst(0));
		} else {
			to.to << lea(ptrB, res.safeLocation(to.child(b), this->result));
		}

		// Spawn the thread!
		to.to << lea(ptrA, fnParams);
		to.to << lea(ptrC, returnType);
		to.to << fnParam(ref);
		to.to << fnParam(ptrA);
		to.to << fnParam(ptrB);
		to.to << fnParam(ptrC);
		to.to << fnParam(thread);
		to.to << fnParam(dataNoFree);
		to.to << fnCall(Ref(e.fnRefs.spawnResult), Size());

		to.to << end(b);
	}

	code::RefSource *Function::threadThunk() {
		using namespace code;

		if (threadThunkRef)
			return threadThunkRef;

		bool needsThunk = false;
		needsThunk |= !result.isBuiltIn();
		for (nat i = 0; i < params.size(); i++)
			needsThunk |= !result.isBuiltIn();

		if (!needsThunk)
			return null;


		Listing l;
		l << prolog();

		Variable resultParam;
		if (result.isValue()) {
			resultParam = l.frame.createPtrParam(result.destructor(), freeOnException);
			l << fnParam(resultParam);
		}

		for (nat i = 0; i < params.size(); i++) {
			const Value &t = params[i];
			if (t.isClass()) {
				Variable v = l.frame.createPtrParam(Ref(engine().fnRefs.release));
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
			Variable t = l.frame.createPtrVar(l.frame.root(), Ref(engine().fnRefs.release));
			l << fnCall(Ref(ref()), Size::sPtr);
			l << mov(t, ptrA);
			// Copy it...
			l << fnParam(ptrA);
			l << fnCall(stdCloneFn(engine(), result), Size::sPtr);
		} else if (result.isValue()) {
			l << fnCall(Ref(ref()), Size::sPtr);

			// We need to create a CloneEnv object.
			Value envType(CloneEnv::type(engine()));
			Variable cloneMem = l.frame.createPtrVar(l.frame.root(), Ref(engine().fnRefs.freeRef), freeOnException);
			Variable cloneEnv = l.frame.createPtrVar(l.frame.root(), Ref(engine().fnRefs.release));
			l << fnParam(Ref(envType.type->typeRef));
			l << fnCall(Ref(engine().fnRefs.allocRef), Size::sPtr);
			l << mov(cloneMem, ptrA);
			l << fnParam(ptrA);
			l << fnCall(envType.defaultCtor(), Size::sPtr);
			l << mov(cloneEnv, ptrA);
			l << mov(cloneMem, intPtrConst(0));

			// Find 'deepCopy'.
			Function *deepCopy = as<Function>(result.type->find(L"deepCopy", valList(2, result, envType)));
			if (!deepCopy)
				throw InternalError(L"The type " + ::toS(result) + L" does not have the required 'deepCopy' member.");

			// Copy by calling 'deepCopy'.
			l << fnParam(resultParam);
			l << fnParam(cloneEnv);
			l << fnCall(Ref(deepCopy->ref()), Size::sPtr);

			l << mov(ptrA, resultParam);
		} else {
			l << fnCall(Ref(ref()), result.size());
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
			lookup = CREATE(DelegatedCode, engine(), code::Ref(*codeRef), lookupRef->getTitle());
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
				lookup = CREATE(DelegatedCode, engine(), code::Ref(*codeRef), lookupRef->getTitle());
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

}
