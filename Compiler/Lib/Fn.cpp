#include "stdafx.h"
#include "Fn.h"
#include "TemplateList.h"
#include "Engine.h"
#include "Exception.h"
#include "Clone.h"
#include "Core/Fn.h"
#include "Core/Str.h"

namespace storm {

	Type *createFn(Str *name, ValueArray *params) {
		return new (name) FnType(name, params);
	}

	Type *fnType(Array<Value> *params) {
		Engine &e = params->engine();
		TemplateList *l = e.cppTemplate(ArrayId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'fnType'.");
		Type *found = as<Type>(to->find(S("Fn"), params));
		if (!found)
			throw InternalError(L"Can not find the function type!");
		return found;
	}

	static void fnCopy(void *mem, FnBase *src) {
		new (Place(mem)) FnBase(*src);
	}

	FnType::FnType(Str *name, ValueArray *params) : Type(name, params->toArray(), typeClass) {
		setSuper(FnBase::stormType(engine));
	}

	Bool FnType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&fnCopy)));

		Array<Value> *fnCall = clone(params);
		fnCall->at(0) = t;
		add(lazyFunction(e, params->at(0), S("call"), fnCall, fnPtr(e, &FnType::callCode, this)));

		return Type::loadAll();
	}

	CodeGen *FnType::callCode() {
		using namespace code;
		Engine &e = engine;

		CodeGen *s = new (e) CodeGen(RunOn());
		Var me = s->l->createParam(valPtr());

		// Result parameter needed?
		Value result = params->at(0);
		Var resultParam;
		if (!result.returnInReg())
			resultParam = s->l->createParam(valPtr());

		// Create parameters.
		Array<Value> *formal = new (e) Array<Value>();
		Array<Operand> *input = new (e) Array<Operand>();
		for (nat i = 1; i < params->count(); i++) {
			formal->push(params->at(i));
			input->push(s->createParam(params->at(i)));
		}

		// Create the thunk if not done already.
		if (!thunk) {
			thunk = new (this) code::RefSource(*identifier() + new (this) Str(L"<thunk>"));
			thunk->set(callThunk(result, formal));
		}

		// Figure out if the first parameter is a TObject and pass it on in case the FnPtr needs to know.
		Operand firstTObj = ptrConst(Offset());
		if (params->count() > 1 && params->at(1).isActor())
			firstTObj = input->at(0);

		*s->l << prolog();

		// Do we need to clone the parameters and the result?
		Var needClone = s->l->createVar(s->l->root(), Size::sByte);
		*s->l << fnParam(me);
		*s->l << fnParam(firstTObj);
		*s->l << fnCall(e.ref(Engine::rFnNeedsCopy), ValType(Size::sByte, false));
		*s->l << mov(needClone, al);

		// Handle parameters.
		Label doCopy = s->l->label();
		Label done = s->l->label();
		*s->l << cmp(needClone, byteConst(0));
		*s->l << jmp(doCopy, ifNotEqual);
		Var fnParamsPlain = createFnCall(s, formal, input, false);
		*s->l << lea(ptrC, fnParamsPlain);
		*s->l << jmp(done);
		*s->l << doCopy;
		Var fnParamsClone = createFnCall(s, formal, input, true);
		*s->l << lea(ptrC, fnParamsClone);
		*s->l << done;

		// Call the function!
		if (!result.returnInReg()) {
			// Value -> call deepCopy if present.
			*s->l << fnParam(me); // b
			*s->l << fnParam(resultParam); // output
			*s->l << fnParam(Ref(thunk)); // thunk
			*s->l << fnParam(ptrC); // params
			*s->l << fnParam(firstTObj); // first
			*s->l << fnCall(e.ref(Engine::rFnCall), valVoid());

			// Call 'deepCopy'
			if (Function *call = result.type->deepCopyFn()) {
				Var env = allocObject(s, CloneEnv::stormType(e));
				*s->l << fnParam(resultParam);
				*s->l << fnParam(env);
				*s->l << fnCall(call->ref(), valVoid());
			}

			*s->l << mov(ptrA, resultParam);
		} else if (result.isBuiltIn() || result.isActor()) {
			// No need to copy!
			Var r;
			if (result != Value()) {
				r = s->l->createVar(s->l->root(), result.size());
				*s->l << lea(ptrA, r);
			} else {
				*s->l << mov(ptrA, ptrConst(Offset()));
			}
			*s->l << fnParam(me); // b
			*s->l << fnParam(ptrA); // output
			*s->l << fnParam(Ref(thunk)); // thunk
			*s->l << fnParam(ptrC); // params
			*s->l << fnParam(firstTObj); // first
			*s->l << fnCall(e.ref(Engine::rFnCall), valVoid());
			if (result != Value())
				*s->l << mov(asSize(ptrA, result.size()), r);
		} else {
			// Class! Call 'core.clone'.
			Var r = s->l->createVar(s->l->root(), result.size());
			*s->l << lea(ptrA, r);
			*s->l << fnParam(me); // b
			*s->l << fnParam(ptrA); // output
			*s->l << fnParam(Ref(thunk)); // thunk
			*s->l << fnParam(ptrC); // params
			*s->l << fnParam(firstTObj); // first
			*s->l << fnCall(e.ref(Engine::rFnCall), valVoid());

			if (Function *call = cloneFn(result.type)) {
				*s->l << fnParam(r);
				*s->l << fnCall(call->ref(), valPtr());
			} else {
				*s->l << mov(ptrA, r);
			}
		}

		*s->l << epilog();
		*s->l << ret(result.valTypeRet());

		return s;
	}

	void CODECALL fnCallRaw(FnBase *b, void *output, os::CallThunk thunk, void **params, TObject *first) {
		// TODO: We can provide a single CloneEnv so that all parameters are cloned uniformly.
		os::FnCallRaw call(params, thunk);
		b->callRaw(output, call, first, null);
	}

	class RefFnTarget : public FnTarget {
	public:
		RefFnTarget(code::Ref ref) : ref(ref) {}

		// What we refer to.
		code::Ref ref;

		// Clone.
		virtual void cloneTo(void *to, size_t size) const {
			assert(size >= sizeof(*this));
			new (to) RefFnTarget(ref);
		}

		// Get the pointer.
		virtual const void *ptr() const {
			return ref.address();
		}


		// Get label.
		virtual void toS(StrBuf *to) const {
			*to << ref.title();
		}
	};

	FnBase *pointer(Function *target) {
		return pointer(target, null);
	}

	FnBase *pointer(Function *target, RootObject *thisPtr) {
		Array<Value> *p = clone(target->params);
		p->insert(0, target->result);
		Type *t = fnType(p);

		Thread *thread = null;
		RunOn runOn = target->runOn();
		if (runOn.state == RunOn::named)
			thread = runOn.thread->thread();

		void *mem = runtime::allocObject(sizeof(FnBase), t);
		FnBase *c = new (Place(mem)) FnBase(RefFnTarget(target->ref()),
											thisPtr,
											target->isMember(),
											thread);
		runtime::setVTable(c);
		return c;
	}

	FnBase *fnCreateRaw(Type *type, code::RefSource *to, Thread *thread, RootObject *thisPtr, Bool memberFn) {
		void *mem = runtime::allocObject(sizeof(FnBase), type);
		FnBase *c = new (Place(mem)) FnBase(RefFnTarget(to), thisPtr, memberFn, thread);
		runtime::setVTable(c);
		return c;
	}

}
