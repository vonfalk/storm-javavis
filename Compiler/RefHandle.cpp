#include "stdafx.h"
#include "RefHandle.h"
#include "Function.h"
#include "Engine.h"
#include "Utils/Memory.h"
#include "Code/Listing.h"

namespace storm {

	RefHandle::RefHandle(code::Content *content) : content(content) {}

	void RefHandle::setCopyCtor(code::Ref ref) {
		if (copyRef)
			copyRef->disable();
		copyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, copyFn), ref, content);
	}

	void RefHandle::setDestroy(code::Ref ref) {
		if (destroyRef)
			destroyRef->disable();
		destroyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, destroyFn), ref, content);
	}

	void RefHandle::setDeepCopy(code::Ref ref) {
		if (deepCopyRef)
			deepCopyRef->disable();
		deepCopyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, deepCopyFn), ref, content);
	}

	void RefHandle::setToS(code::Ref ref) {
		if (toSRef)
			toSRef->disable();
		toSRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, toSFn), ref, content);
	}

	void RefHandle::setToS(Function *fn) {
		if (toSRef)
			toSRef->disable();

		// We do not need references for this to work. The code and all its metadata will be kept
		// alive if we store a pointer to the code in the 'toSFn' variable.
		code::Binary *code = toSThunk(fn);
		toSFn = (ToSFn)code->address();
	}

	void RefHandle::setHash(code::Ref ref) {
		if (hashRef)
			hashRef->disable();
		hashRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, hashFn), ref, content);
	}

	void RefHandle::setEqual(code::Ref ref) {
		if (equalRef)
			equalRef->disable();
		equalRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, equalFn), ref, content);
	}

	void RefHandle::setLess(code::Ref ref) {
		if (lessRef)
			lessRef->disable();
		lessRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, lessFn), ref, content);
	}


	code::Binary *toSThunk(Function *fn) {
		assert(fn->params->count() == 2);
		Value valParam = fn->params->at(1);

		using namespace code;
		Engine &e = fn->engine();
		TypeDesc *ptr = e.ptrDesc();
		Listing *l = new (fn) Listing(false, ptr);

		code::Var valRef = l->createParam(ptr);
		code::Var strBuf = l->createParam(ptr);

		*l << prolog();
		*l << fnParam(ptr, strBuf);
		if (valParam.ref) {
			*l << fnParam(valParam.desc(e), valRef);
		} else {
			*l << fnParamRef(valParam.desc(e), valRef);
		}
		*l << fnCall(Ref(fn->ref()), true, ptr, ptrA);
		*l << fnRet(ptrA);

		return new (fn) code::Binary(fn->engine().arena(), l);
	}

	template <class T>
	static Nat wrapHash(const void *obj) {
		const T *&o = *(const T **)obj;
		return o->hash();
	}

	template <class T>
	static Bool wrapEquals(const void *a, const void *b) {
		const T *&oa = *(const T **)a;
		const T *&ob = *(const T **)b;
		return *oa == *ob;
	}

	template <class T>
	static Bool wrapLess(const void *a, const void *b) {
		const T *&oa = *(const T **)a;
		const T *&ob = *(const T **)b;
		return *oa < *ob;
	}


	static void populateStr(Handle *h) {
		h->hashFn = &wrapHash<Str>;
		h->equalFn = &wrapEquals<Str>;
		h->lessFn = &wrapLess<Str>;
	}

	void populateHandle(Type *type, Handle *h) {
		Engine &e = type->engine;
		if (type == Str::stormType(e))
			populateStr(h);
	}
}
