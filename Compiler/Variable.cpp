#include "stdafx.h"
#include "Variable.h"
#include "Type.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Lib/Fn.h"
#include "Exception.h"
#include "Engine.h"

namespace storm {

	static void checkType(Value type) {
		if (!type.type)
			throw TypedefError(L"Unable to create a variable of type 'void'.");
	}

	Variable::Variable(Str *name, Value type) :
		Named(name), type(type.asRef(false)) {

		checkType(type);
	}

	Variable::Variable(Str *name, Value type, Type *member) :
		Named(name, new (name) Array<Value>(1, Value(member))), type(type) {

		checkType(type);
	}


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
		Variable(name, type), owner(thread), initializer(initializer), hasArray(false) {

		FnType *fnType = as<FnType>(runtime::typeOf(initializer));
		if (!fnType)
			throw RuntimeError(L"Invalid type of the initializer passed to GlobalVar. Must be a function pointer.");
		if (fnType->params->count() != 1)
			throw RuntimeError(L"An initializer provided to GlobalVar may not take parameters.");

		hasArray = type.isValue() || type.isBuiltIn();
	}

	Bool GlobalVar::accessibleFrom(RunOn thread) {
		return thread.canRun(RunOn(owner));
	}

	void GlobalVar::create() {
		// Already created?
		if (!initializer)
			return;

		FnType *fnType = as<FnType>(runtime::typeOf(initializer));

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
		os::Thread initOn = owner->thread()->thread();
		if (initOn == os::Thread::current()) {
			// No thread call needed.
			call.callRaw(callFn->ref().address(), true, null, outPtr);
		} else {
			// Call on the specified thread.
			os::FutureSema<os::Sema> future;
			os::UThread::spawnRaw(callFn->ref().address(), true, null, call, future, outPtr, &initOn);
			future.result();
		}

		// Mark ourselves as initialized.
		initializer = null;
	}

	void GlobalVar::compile() {
		create();
	}

	void *GlobalVar::dataPtr() {
		if (atomicRead(initializer) != null) {
			// We might need to initialize! Call 'create' on the proper thread to do that. Note:
			// 'create' will not initialize us again if initialization was done already, so we will
			// be safe even if two threads happen to call 'dataPtr' for the first time more or less
			// simultaneously.
			const os::Thread &t = TObject::thread->thread();
			if (t != os::Thread::current()) {
				GlobalVar *me = this;
				os::Future<void, Semaphore> f;
				os::FnCall<void, 1> p = os::fnCall().add(me);
				os::UThread::spawn(address(&GlobalVar::create), true, p, f, &t);
				f.result();
			} else {
				create();
			}
		}

		if (hasArray) {
			GcArray<byte> *a = (GcArray<byte> *)data;
			return a->v;
		} else {
			return &data;
		}
	}

	Str *GlobalVar::strValue() const {
		const Handle &h = type.type->handle();
		if (h.toSFn) {
			StrBuf *out = new (this) StrBuf();
			void *ptr = const_cast<GlobalVar *>(this)->dataPtr();
			(*h.toSFn)(ptr, out);
			return out->toS();
		} else {
			return new (this) Str(S("<no string representation>"));
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

}
