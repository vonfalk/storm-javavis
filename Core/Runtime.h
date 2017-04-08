#pragma once
#include "GcArray.h"

namespace storm {
	struct GcType;
	struct GcCode;
	class GcWatch;
	class RootObject;
	class CloneEnv;
	class StdRequest;

	namespace runtime {

		/**
		 * This file declares the interface to the runtime that code in Core/ needs, but can not be
		 * implemented here, as the implementation differs depending on if Core/ is linked to the
		 * Compiler/ or a shared library. For shared libraries, include SharedLib.h from *one*
		 * source file, to get the implementation suitable for shared libraries (which just
		 * delegates calls to the compiler linked at runtime).
		 */

		// Get the Storm type description for a C++ type given its ID (used by the generated type information code).
		Type *cppType(Engine &e, Nat id);

		// Variadic argument variant of 'cppTemplate'.
		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list params);

		// Get the Storm type description for a C++ template, given its ID and template type ids (we
		// do not support eg. Array<Array<X>> from C++.
		inline Type *cppTemplate(Engine &e, Nat id, Nat count, ...) {
			Type *r;
			va_list l;
			va_start(l, count);
			r = cppTemplateVa(e, id, count, l);
			va_end(l);
			return r;
		}

		// Get a type handle for 'type'.
		const Handle &typeHandle(Type *t);

		// Get the handle for 'void'.
		const Handle &voidHandle(Engine &e);

		// Get the type of an allocation.
		Type *typeOf(const RootObject *o);

		// Get the GcType for an allocation.
		const GcType *gcTypeOf(const void *alloc);

		// Get the name of a type.
		Str *typeName(Type *t);

		// Is type A an instance of type B?
		bool isA(const RootObject *a, const Type *b);

		// Get the engine object for an allocation.
		Engine &allocEngine(const RootObject *o);

		// Allocate some raw memory for some data.
		void *allocRaw(Engine &e, const GcType *type);

		// Allocate some non-moving raw memory for some data.
		void *allocStaticRaw(Engine &e, const GcType *type);

		// Allocate an object of the given type (size used for sanity checking).
		void *allocObject(size_t size, Type *type);

		// Allocate an array of objects.
		void *allocArray(Engine &e, const GcType *type, size_t count);

		template <class T>
		inline GcArray<T> *allocArray(Engine &e, const GcType *type, size_t count) {
			return (GcArray<T> *)allocArray(e, type, count);
		}

		// Allocate a weak array of pointers.
		void *allocWeakArray(Engine &e, size_t count);

		template <class T>
		inline GcWeakArray<T> *allocWeakArray(Engine &e, size_t count) {
			return (GcWeakArray<T> *)allocWeakArray(e, count);
		}

		// Allocate a code segment with at least 'code' bytes of memory for the code, and 'refs'
		// entries for references.
		void *allocCode(Engine &e, size_t code, size_t refs);

		// Get the code size of a previous code allocation (references are not counted).
		size_t codeSize(const void *code);

		// Get the references for a block of code.
		GcCode *codeRefs(void *code);

		// Update all pointers in a code allocation. This copies all pointer values from the
		// 'codeRef' section into the data section.
		void codeUpdatePtrs(void *code);

		// Re-set the vtable of an object to what it should be.
		void setVTable(RootObject *object);

		// Get the thread group to use for all threads.
		os::ThreadGroup &threadGroup(Engine &e);

		// Create a GcWatch object.
		GcWatch *createWatch(Engine &e);

		// Post an IO-request for standard in/out/error.
		void postStdRequest(Engine &e, StdRequest *request);

		// Clone any heap-allocated object from C++.
		RootObject *CODECALL cloneObject(RootObject *obj);
		RootObject *CODECALL cloneObjectEnv(RootObject *obj, CloneEnv *env);

	}
}
