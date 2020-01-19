#include "stdafx.h"
#include "Future.h"
#include "PtrThrowable.h"

namespace os {

	FutureBase::FutureBase() : resultPosted(resultEmpty), resultRead(readNone) {}

	FutureBase::~FutureBase() {
		destroy(null);
	}

	void FutureBase::destroy(void *ptrStorage) {
		// Warn about uncaught exceptions if we didn't throw it yet.
		if (resultRead == readNone) {
			switch (atomicRead(resultPosted)) {
			case resultError:
				try {
					throwError();
				} catch (const ::Exception &e) {
					PLN(L"Unhandled exception from abandoned future: " << e);
				} catch (const PtrThrowable *e) {
					PLN(L"Unhandled exception from abandoned future: " << e->toCStr());
				} catch (...) {
					PLN(L"Unknown unhandled exception from abandoned future.");
				}
				break;
			case resultErrorPtr:
				if (ptrStorage)
					PLN(L"Unhandled exception from abandoned future: " << ((PtrThrowable *)ptrStorage)->toCStr());
				else
					PLN(L"Unknown unhandled exception from abandoned future.");
				break;
			}

			resultRead = readOnce;
		}
	}

	void FutureBase::result(void *ptrStorage) {
		// Block the first thread, and allow any subsequent threads to enter.
		wait();
		atomicWrite(resultRead, readOnce);
		switch (atomicRead(resultPosted)) {
		case resultError:
			throwError();
		case resultErrorPtr:
			throw (PtrThrowable *)ptrStorage;
		}
	}

	void FutureBase::detach() {
		atomicWrite(resultRead, readDetached);
	}

	bool FutureBase::anyPosted() {
		return atomicRead(resultPosted) != resultEmpty;
	}

	bool FutureBase::dataPosted() {
		return atomicRead(resultPosted) == resultValue;
	}

	void FutureBase::posted() {
		nat p = atomicCAS(resultPosted, resultEmpty, resultError);
		assert(p == 0, L"A future may not be used more than once! this=" + ::toHex(this));
		notify();
	}

	void FutureBase::error(void **ptrStorage) {
		try {
			throw;
		} catch (const PtrThrowable *exception) {
			// Save the exception pointer directly. It may be GC:d.
			nat p = atomicCAS(resultPosted, resultEmpty, resultErrorPtr);
			assert(p == resultError, L"A future may not be used more than once! this=" + ::toHex(this));
			*ptrStorage = (void *)exception;
		} catch (...) {
			// Fall back on exception_ptr or similar.
			nat p = atomicCAS(resultPosted, resultEmpty, resultError);
			assert(p == resultError, L"A future may not be used more than once! this=" + ::toHex(this));
			saveError();
		}
		notify();
	}

#ifdef CUSTOM_EXCEPTION_PTR

#ifndef X86
#error "Only X86 is supported now. Please revise the code or use a compiler with support for exception_ptr"
#endif

	/**
	 * Implementation more or less copied from Boost exception_ptr.
	 */

	const nat cppExceptionCode = 0xE06D7363;
	const nat cppExceptionMagic = 0x19930520;
	const nat cppExceptionParams = 3;
#if _MSC_VER==1310
	const nat exceptionInfoOffset = 0x74;
#elif (_MSC_VER == 1400 || _MSC_VER == 1500)
	const nat exceptionInfoOffset = 0x80;
#else
#error "Unknown MSC version."
#endif

	struct DummyException {};
	typedef int(DummyException::*CopyCtor)(void * src);
	typedef int(DummyException::*VirtualCopyCtor)(void * src, void * dst); // virtual base
	typedef void(DummyException::*Dtor)();

	union CppCopyCtor {
		CopyCtor normal;
		VirtualCopyCtor virtualCtor;
	};

	enum CppTypeFlags {
		simpleType = 1,
		virtualBaseType = 4,
	};

	struct CppTypeInfo {
		unsigned flags;
		std::type_info * typeInfo;
		int thisOffset;
		int vbaseDescr;
		int vbaseOffset;
		unsigned long size;
		CppCopyCtor copyCtor;
	};

	struct CppTypeInfoTable {
		unsigned count;
		const CppTypeInfo *info[1];
	};

	struct CppExceptionType {
		unsigned flags;
		Dtor dtor;
		void(*handler)();
		const CppTypeInfoTable *table;
	};

	static const CppTypeInfo *getCppTypeInfo(const CppExceptionType *t) {
		const CppTypeInfo *info = t->table->info[0];
		assert(info);
		return info;
	}

	static void copyException(void *to, void *from, const CppTypeInfo *info) {
		bool copyCtor = (info->flags & simpleType) == 0;
		copyCtor &= info->copyCtor.normal != null;

		if (copyCtor) {
			DummyException *p = (DummyException *)to;
			if (info->flags & virtualBaseType)
				(p->*(info->copyCtor.virtualCtor))(from, to);
			else
				(p->*(info->copyCtor.normal))(from);
		} else {
			memmove(to, from, info->size);
		}
	}

	FutureBase::Cloned::Cloned() : data(null), type(null) {}

	FutureBase::Cloned::~Cloned() {
		clear();
	}

	void FutureBase::Cloned::clear() {
		if (data) {
			if (type->dtor) {
				DummyException *p = (DummyException *)data;
				(p->*(type->dtor))();
			}
			free(data);
		}
		data = null;
		type = null;
	}

	void FutureBase::Cloned::rethrow() {
		// It is safe to keep stuff at the stack here. catch-blocks are executed
		// on top of this stack, ie this stack frame will not be reclaimed until
		// the catch-blocks have been executed.
		const CppTypeInfo *info = getCppTypeInfo(type);
		void *dst = _alloca(info->size);
		copyException(dst, data, info);
		ULONG_PTR args[cppExceptionParams];
		args[0] = cppExceptionMagic;
		args[1] = (ULONG_PTR)dst;
		args[2] = (ULONG_PTR)type;
		RaiseException(cppExceptionCode, EXCEPTION_NONCONTINUABLE, cppExceptionParams, args);
	}

	void FutureBase::Cloned::set(void *src, const CppExceptionType *type) {
		clear();

		this->type = type;
		const CppTypeInfo *ti = getCppTypeInfo(type);
		data = malloc(ti->size);
		try {
			copyException(data, src, ti);
		} catch (...) {
			free(data);
			throw;
		}
	}

	static bool isCppException(EXCEPTION_RECORD *record) {
		return record
			&& record->ExceptionCode == cppExceptionCode
			&& record->NumberParameters == cppExceptionParams
			&& record->ExceptionInformation[0] == cppExceptionMagic;
	}


	int FutureBase::filter(EXCEPTION_POINTERS *info) {
		EXCEPTION_RECORD *record = info->ExceptionRecord;

		if (!isCppException(record)) {
			WARNING(L"Not a C++ exception!");
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if (!record->ExceptionInformation[2]) {
			// It was a re-throw. We need to go look up the exception record from tls!
			byte *t = (byte *)_errno();
			record = *(EXCEPTION_RECORD **)(t + exceptionInfoOffset);
			//record = *reinterpret_cast<EXCEPTION_RECORD **>(reinterpret_cast<char *>(_errno()) + exceptionInfoOffset);
		}

		if (!isCppException(record)) {
			WARNING(L"Not a C++ exception!");
		}
		assert(record->ExceptionInformation[2]);

		errorData.set(
			(void *)record->ExceptionInformation[1],
			(const CppExceptionType *)record->ExceptionInformation[2]);

		return EXCEPTION_EXECUTE_HANDLER;
	}

	void FutureBase::saveError() {
		__try {
			throw;
		} __except (filter(GetExceptionInformation())) {
		}
	}


	void FutureBase::throwError() {
		errorData.rethrow();
	}

#else

	void FutureBase::throwError() {
		std::rethrow_exception(errorData);
	}

	void FutureBase::saveError() {
		errorData = std::current_exception();
	}

#endif

}
