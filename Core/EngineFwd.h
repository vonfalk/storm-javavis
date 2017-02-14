#pragma once
#include "OS/UThread.h"

namespace storm {
	struct GcType;
	struct GcCode;
	class GcWatch;
	class RootObject;
	class CloneEnv;
	class StdRequest;

	/**
	 * Forward calls to the engine through dll-boundaries using function pointers in this
	 * struct. This covers all functions present in storm::runtime.
	 */
	struct EngineFwd {
		// Everything present in storm::runtime:
		Type *(*cppType)(Engine &e, Nat id);
		Type *(*cppTemplateVa)(Engine &e, Nat id, Nat count, va_list params);
		const Handle &(*typeHandle)(Type *t);
		const Handle &(*voidHandle)(Engine &e);
		Type *(*typeOf)(const RootObject *o);
		const GcType *(*gcTypeOf)(const void *alloc);
		Str *(*typeName)(Type *t);
		Bool (*isA)(const RootObject *a, const Type *b);
		Engine &(*allocEngine)(const RootObject *o);
		void *(*allocObject)(size_t size, Type *type);
		void *(*allocArray)(Engine &e, const GcType *type, size_t count);
		void *(*allocWeakArray)(Engine &e, size_t count);
		void *(*allocCode)(Engine &e, size_t code, size_t refs);
		size_t (*codeSize)(const void *code);
		GcCode *(*codeRefs)(void *code);
		void (*setVTable)(RootObject *object);
		os::ThreadGroup &(*threadGroup)(Engine &e);
		GcWatch *(*createWatch)(Engine &e);
		void (*attachThread)(Engine &e);
		void (*detachThread)(Engine &e, const os::Thread &thread);
		void (*reattachThread)(Engine &e, const os::Thread &thread);
		void (*postStdRequest)(Engine &e, StdRequest *request);
		RootObject *(*cloneObject)(RootObject *obj);
		RootObject *(*cloneObjectEnv)(RootObject *obj, CloneEnv *env);

		// Additional functions required.
		Thread *(*getThread)(Engine &e, const DeclThread *decl);
		os::ThreadData *(*getCurrentThreadData)();
		void (*setCurrentThreadData)(os::ThreadData *data);
		os::UThreadState *(*getCurrentUThreadState)();
		void (*setCurrentUThreadState)(os::UThreadState *state);
		void (*threadCreated)();
		void (*threadTerminated)();

		// Completely different type to catch errors when using initializer lists.
		Float dummy;
	};

	// Create an EngineFwd for this module with all pointer properly filled.
	const EngineFwd &engineFwd();

#ifndef STORM_COMPILER

	// Set the EngineFwd to use for this instance.
	void useFwd(const EngineFwd &use);

#endif
}
