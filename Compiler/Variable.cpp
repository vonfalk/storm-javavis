#include "stdafx.h"
#include "Variable.h"
#include "Type.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Lib/Fn.h"
#include "Exception.h"
#include "Engine.h"

namespace storm {

	Variable::Variable(Str *name, Value type) :
		Named(name), type(type.asRef(false)) {}

	Variable::Variable(Str *name, Value type, Type *member) :
		Named(name, new (name) Array<Value>(1, Value(member))), type(type) {}


	void Variable::toS(StrBuf *to) const {
		*to << type << S(" ");
		Named::toS(to);
	}


	/**
	 * Member variable.
	 */

	MemberVar::MemberVar(Str *name, Value type, Type *member) : Variable(name, type, member) {
		hasLayout = false;
	}

	Offset MemberVar::offset() const {
		if (!hasLayout) {
			owner()->doLayout();
		}
		return off;
	}

	void MemberVar::setOffset(Offset to) {
		hasLayout = true;
		off = to;
	}

	Type *MemberVar::owner() const {
		assert(!params->empty());
		return params->at(0).type;
	}


	/**
	 * Global variable.
	 */

	GlobalVar::GlobalVar(Str *name, Value type, NamedThread *thread, FnBase *initializer) :
		Variable(name, type), owner(thread) {

		FnType *fnType = as<FnType>(runtime::typeOf(initializer));
		if (!fnType)
			throw RuntimeError(L"Invalid type of the initializer passed to GlobalVar. Must be a function pointer.");
		if (fnType->params->count() != 1)
			throw RuntimeError(L"An initializer provided to GlobalVar may not take parameters.");

		// Find the 'call' function so that we may call that.
		Function *callFn = as<Function>(fnType->find(S("call"), thisPtr(fnType), engine().scope()));
		if (!callFn)
			throw RuntimeError(L"Can not find 'call()' in the provided function pointer. Is the signature correct?");

		// Check the return type.
		if (!type.canStore(callFn->result))
			throw RuntimeError(L"The global variable " + ::toS(name) +
							L" can not store the type returned from the initializer. Expected " +
							::toS(type) + L", got " + ::toS(callFn->result) + L".");


		void *outPtr = &data;
		hasArray = type.isValue() || type.isBuiltIn();
		if (hasArray) {
			// Create the storage for the data.
			GcArray<byte> *d = (GcArray<byte> *)runtime::allocArray(engine(), type.type->gcArrayType(), 1);
			data = d;
			outPtr = d->v;
		}

		// Call the function using its thunk (additional parameter just in case).
		void *params[2] = { &initializer, null };
		os::FnCallRaw call(params, callFn->callThunk());

		// Call on the specified thread.
		os::Thread initOn = thread->thread()->thread();
		if (initOn == os::Thread::current()) {
			// No thread call needed.
			call.callRaw(callFn->ref().address(), true, null, outPtr);
		} else {
			// Call on the specified thread.
			os::FutureSema<os::Sema> future;
			os::UThread::spawnRaw(callFn->ref().address(), true, null, call, future, outPtr, &initOn);
			future.result();
		}
	}

	void GlobalVar::toS(StrBuf *to) const {
		Variable::toS(to);
		*to << S(" on ") << owner->identifier();
		const Handle &h = type.type->handle();
		if (h.toSFn) {
			*to << S(" = ");
			// Sorry...
			void *ptr = const_cast<GlobalVar *>(this)->dataPtr();
			(*h.toSFn)(ptr, to);
		}
	}

	void *GlobalVar::dataPtr() {
		if (hasArray) {
			GcArray<byte> *a = (GcArray<byte> *)data;
			return a->v;
		} else {
			return &data;
		}
	}

	Bool GlobalVar::accessibleFrom(RunOn thread) {
		return thread.canRun(RunOn(owner));
	}

}
