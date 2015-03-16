#include "stdafx.h"
#include "TypeDtor.h"
#include "Code/VTable.h"

namespace storm {

	TypeDefaultDtor::TypeDefaultDtor(Type *owner)
		: Function(Value(), Type::DTOR, vector<Value>(1, Value::thisPtr(owner))) {

		Function *before = null;
		Type *super = owner->super();

		if (super) {
			// We may need to run another function first...
			before = super->destructor();
		}

		generateCode(owner, before);
	}

	void TypeDefaultDtor::generateCode(Type *type, Function *before) {
		using namespace code;

		Listing l;
		Variable dest = l.frame.createPtrParam();

		l << prolog();

		vector<Auto<TypeVar> > vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = v->varType;
			code::Value dtor = t.destructor();
			if (dtor.type() != code::Value::tNone) {
				l << mov(ptrA, dest);
				l << add(ptrA, intPtrConst(v->offset()));
				if (t.isClass())
					l << mov(ptrA, ptrRel(ptrA));

				l << fnParam(ptrA);
				l << fnCall(dtor, Size());
			}
		}

		if (before) {
			l << fnParam(dest);
			l << fnCall(before->directRef(), Size());
		}

		l << epilog();
		l << ret(Size());

		setCode(steal(CREATE(DynamicCode, this, l)));
	}


	/**
	 * We need a struct in order to be able to use __thiscall.
	 */
	struct RedirectDtor {
#if defined(X86) && defined(VS)
		// on VS, there is a hidden int parameter that is 1 if we shall run
		// delete on the memory after destruction. We fall back on the operator
		// delete in Object for this, since it is most likely an object in this
		// case (but we don't know). It will be easy to track down any errors at least.
		void __thiscall redirect(int del) {
			// Get the Storm vtable.
			void *cppVTable = code::vtableOf(this);
			void **vtable = (void **)code::vtableExtra(cppVTable);

			// Get the dtor (always in slot 0).
			typedef void (CODECALL *Dtor)(void *);
			Dtor dtor = (Dtor)vtable[0];
			assert(dtor, "Someone forgot to set the Storm dtor!");
			(*dtor)(this);

			if (del)
				Object::operator delete(this);
		}
#endif
		// Some data, to make sure we're a nonzero size.
		void *dummy;
	};

	void *dtorRedirect() {
		return address(&RedirectDtor::redirect);
	}


#if defined(X86) && defined(VS)
	/**
	 * On VS, we need to add a hidden parameter that tells the delete fn
	 * that we do not want to run operator delete afterwards. Aside from
	 * this, we need to use the __thiscall calling convention, ie the first
	 * parameter is in ecx.
	 */
	Code *wrapRawDestructor(Engine &e, void *ptr) {
		using namespace code;
		// Hacky version:
		Listing l;
		// Get this ptr.
		l << mov(ptrC, ptrRel(ptrStack, Offset::sPtr));
		// Duplicate return address.
		l << push(ptrRel(ptrStack));
		// Store the zero as the parameter.
		l << mov(ptrRel(ptrStack, Offset::sPtr), ptrConst(0));
		// Run!
		l << jmp(ptrConst(ptr));

		// Simple version:
		// l << mov(ptrC, ptrRel(ptrStack, Offset::sPtr));
		// // This tells that we are _not_ running operator delete afterwards.
		// l << push(intPtrConst(0));
		// l << call(ptrConst(ptr), Size());
		// l << ret(Size());
		return CREATE(DynamicCode, e, l);
	}
#endif


}
