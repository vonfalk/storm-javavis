#include "stdafx.h"
#include "RefHandle.h"
#include "Function.h"
#include "Engine.h"
#include "Utils/Memory.h"
#include "Code/Listing.h"

namespace storm {

	RefHandle::RefHandle(code::Content *content) : content(content) {}

	void RefHandle::setCopyCtor(code::Ref ref) {
		copyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, copyFn), ref, content);
	}

	void RefHandle::setDestroy(code::Ref ref) {
		destroyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, destroyFn), ref, content);
	}

	void RefHandle::setDeepCopy(code::Ref ref) {
		deepCopyRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, deepCopyFn), ref, content);
	}

	void RefHandle::setToS(code::Ref ref) {
		toSRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, toSFn), ref, content);
	}

	void RefHandle::setToS(Function *fn) {
		// We do not need references for this to work. The code and all its metadata will be kept
		// alive if we store a pointer to the code in the 'toSFn' variable.
		code::Binary *code = toSThunk(fn);
		toSFn = (ToSFn)code->address();
	}

	void RefHandle::setHash(code::Ref ref) {
		hashRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, hashFn), ref, content);
	}

	void RefHandle::setEqual(code::Ref ref) {
		equalRef = new (this) code::MemberRef(this, OFFSET_OF(RefHandle, equalFn), ref, content);
	}


	code::Binary *toSThunk(Function *fn) {
		assert(fn->params->count() == 2);
		Value valParam = fn->params->at(1);

		using namespace code;
		Listing *l = new (fn) Listing();

		code::Var valRef = l->createParam(valPtr());
		code::Var strBuf = l->createParam(valPtr());

		*l << prolog();
		*l << fnParam(strBuf);
		if (valParam.ref) {
			*l << fnParam(valRef);
		} else {
			*l << fnParamRef(valRef, valParam.size(), valParam.copyCtor());
		}
		*l << fnCall(Ref(fn->ref()), valPtr());
		*l << epilog();
		*l << ret(valPtr());

		return new (fn) code::Binary(fn->engine().arena(), l);
	}
}
