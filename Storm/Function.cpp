#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "TypeDtor.h"
#include "Exception.h"
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
		to.to << lea(ptrC, fnParams);
		to.to << fnParam(ptrC);
		to.to << fnParam(ptrA);
		to.to << fnCall(Ref(e.fnRefs.fnParamsCtor), Size());

		// Add all parameters.
		for (nat i = 0; i < params.size(); i++) {
			const Value &v = this->params[i];

			if (v.isClass()) {
				TODO(L"Do a deep copy of this object!");
				to.to << lea(ptrC, fnParams);
				to.to << lea(ptrA, params[i]);
				to.to << fnParam(ptrC);
				to.to << fnParam(Ref(e.fnRefs.addRef));
				to.to << fnParam(Ref(e.fnRefs.freeRef));
				to.to << fnParam(natConst(v.size()));
				to.to << fnParam(ptrA);
				to.to << fnCall(Ref(e.fnRefs.fnParamsAdd), Size());
			} else if (v.ref) {
				// No need for copy ctors!
				to.to << lea(ptrC, fnParams);
				to.to << fnParam(ptrC);
				to.to << fnParam(intPtrConst(0));
				to.to << fnParam(intPtrConst(0));
				to.to << fnParam(natConst(v.size()));
				to.to << fnParam(params[i]);
				to.to << fnCall(Ref(e.fnRefs.fnParamsAdd), Size());
			} else {
				code::Value dtor = v.destructor();
				if (dtor.empty())
					dtor = intPtrConst(0);
				to.to << lea(ptrC, fnParams);
				to.to << lea(ptrA, params[i]);
				to.to << fnParam(ptrC);
				to.to << fnParam(v.copyCtor());
				to.to << fnParam(dtor);
				to.to << fnParam(natConst(v.size()));
				to.to << fnParam(ptrA);
				to.to << fnCall(Ref(e.fnRefs.fnParamsAdd), Size());
			}
		}

		// Describe the return type.
		Variable returnType = createBasicTypeInfo(to.child(b), this->result);

		// Set the thread data to null, so that we do not double-free it if
		// the call returns with an exception.
		Variable dataNoFree = to.frame.createPtrVar(b);
		to.to << mov(dataNoFree, data);
		to.to << mov(data, intPtrConst(0));

		// Where shall we store the result (store the pointer there in ptrB);
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
		to.to << fnCall(Ref(e.fnRefs.spawn), Size());

		// Wait for the result...
		TODO(L"Make a copy of the result in the other thread if needed.");

		to.to << end(b);
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
