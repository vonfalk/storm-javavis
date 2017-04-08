#include "stdafx.h"
#include "Runtime.h"
#include "Gc.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/Io/StdStream.h"
#include "Code/Refs.h"
#include "StdIoThread.h"

namespace storm {
	namespace runtime {

		/**
		 * Implements the functions declared in 'Core/Runtime.h' for the compiler.
		 */

		Type *cppType(Engine &e, Nat id) {
			return e.cppType(id);
		}

		Type *cppTemplateVa(Engine &e, Nat id, Nat count, va_list l) {
			const nat maxCount = 16;
			assert(count < maxCount, L"Too many template parameters used: " + ::toS(count) + L" max " + ::toS(maxCount));

			TemplateList *tList = e.cppTemplate(id);
			if (!tList)
				return null;

			Nat params[maxCount];
			for (nat i = 0; i < count; i++)
				params[i] = va_arg(l, Nat);

			return tList->find(params, count);
		}

		const Handle &typeHandle(Type *t) {
			return t->handle();
		}

		const Handle &voidHandle(Engine &e) {
			return e.voidHandle();
		}

		Type *typeOf(const RootObject *o) {
			return Gc::typeOf(o)->type;
		}

		Str *typeName(Type *t) {
			return t->name;
		}

		const GcType *gcTypeOf(const void *alloc) {
			return Gc::typeOf(alloc);
		}

		bool isA(const RootObject *a, const Type *t) {
			return typeOf(a)->chain->isA(t);
		}

		Engine &allocEngine(const RootObject *o) {
			return typeOf(o)->engine;
		}

		void *allocRaw(Engine &e, const GcType *type) {
			return e.gc.alloc(type);
		}

		void *allocStaticRaw(Engine &e, const GcType *type) {
			return e.gc.allocStatic(type);
		}

		void *allocObject(size_t size, Type *type) {
			const GcType *t = type->gcType();
			assert(size <= t->stride, L"Invalid type description found! " + ::toS(size) + L" vs " + ::toS(t->stride));
			return type->engine.gc.alloc(t);
		}

		void *allocArray(Engine &e, const GcType *type, size_t count) {
			return e.gc.allocArray(type, count);
		}

		void *allocWeakArray(Engine &e, size_t count) {
			return e.gc.allocWeakArray(count);
		}

		GcWatch *createWatch(Engine &e) {
			return e.gc.createWatch();
		}

		void *allocCode(Engine &e, size_t code, size_t refs) {
			return e.gc.allocCode(code, refs);
		}

		size_t codeSize(const void *code) {
			return Gc::codeSize(code);
		}

		GcCode *codeRefs(void *code) {
			return Gc::codeRefs(code);
		}

		void codeUpdatePtrs(void *code) {
			code::updatePtrs(code, Gc::codeRefs(code));
		}

		void setVTable(RootObject *object) {
			typeOf(object)->vtable->insert(object);
		}

		os::ThreadGroup &threadGroup(Engine &e) {
			return e.threadGroup;
		}

		void attachThread(Engine &e) {
			e.gc.attachThread();
		}

		void detachThread(Engine &e, const os::Thread &thread) {
			e.gc.detachThread(thread);
		}

		void reattachThread(Engine &e, const os::Thread &thread) {
			e.gc.reattachThread(thread);
		}

		void postStdRequest(Engine &e, StdRequest *request) {
			e.stdIo()->post(request);
		}

		RootObject *cloneObject(RootObject *obj) {
			if (obj == null)
				return null;

			// Nothing needs to be done for TObjects.
			if (TObject *t = as<TObject>(obj))
				return obj;

			return cloneObjectEnv(obj, new (obj) CloneEnv());
		}

		RootObject *cloneObjectEnv(RootObject *obj, CloneEnv *env) {
			if (obj == null)
				return null;

			// Nothing needs to be done for TObjects.
			if (TObject *t = as<TObject>(obj))
				return obj;

			Object *src = (Object *)obj;

			if (Object *prev = env->cloned(src))
				return prev;

			Type *t = typeOf(src);
			const GcType *gcType = t->gcType();
			void *mem = t->engine.gc.alloc(gcType);

			Type::CopyCtorFn ctor = t->rawCopyConstructor();
			if (ctor) {
				(*ctor)(mem, src);
			} else {
				// No copy constructor... Well, then we do it the hard way!
				memcpy(mem, src, gcType->stride);
			}

			Object *result = (Object *)mem;
			result->deepCopy(env);

			env->cloned(src, result);
			return result;
		}

	}
}
