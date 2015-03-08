#include "stdafx.h"
#include "Future.h"

#ifdef CUSTOM_EXCEPTION_PTR
// Found in crt/src/tidtable.c
extern "C" void * __cdecl _getptd();
// Look in crt/src/mtdll.h for the _tiddata definition!
#endif

namespace code {

	Future::Future(void *target, nat size)
		: targetPtr(target), size(size), resultSema(0),  hasError(false) {}

	void Future::wait() {
		resultSema.down();
		if (hasError) {
			throwError();
		}
	}

	void Future::result() {
		hasError = false;
		resultSema.up();
	}

#ifdef CUSTOM_EXCEPTION_PTR

#ifndef X86
#error "This only works for X86! Please use a newer compiler with support for exception_ptr."
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

	Future::Cloned::Cloned() : data(null), type(null) {}

	Future::Cloned::~Cloned() {
		clear();
	}

	void Future::Cloned::clear() {
		if (data) {
			DummyException *p = (DummyException *)data;
			(p->*(type->dtor))();
			free(data);
		}
		data = null;
		type = null;
	}

	void Future::Cloned::rethrow() {
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

	void Future::Cloned::set(void *src, const CppExceptionType *type) {
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


	int Future::filter(EXCEPTION_POINTERS *info) {
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

	void Future::error() {
		hasError = true;

		__try {
			throw;
		} __except (filter(GetExceptionInformation())) {
			// Notify result.
			resultSema.up();
		}
	}


	void Future::throwError() {
		errorData.rethrow();
	}

#else

	void Future::throwError() {
		std::rethrow_exception(errorData);
	}

	void Future::error() {
		hasError = true;
		errorData = std::current_exception();
		resultSema.up();
	}

#endif

}
