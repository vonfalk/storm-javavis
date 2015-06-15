#pragma once
#include "FnPtr.h"

namespace storm {
	class Engine;

	/**
	 * Create null functions. Requires that the return value exists and is constructable by the
	 * default ctor.
	 */

	/**
	 * Create objects based on their type. Pointers are assumed to be heap allocated Storm objects.
	 */
	template <class R>
	struct CreateObj {
		static R create() {
			return R();
		}
	};

	template <class R>
	struct CreateObj<R *> {
		static R *create() {
			assert(false, L"Can not return null!");
			// return CREATE(R, *e);
			return null;
		}
	};

	template<>
	struct CreateObj<void> {
		static void create() {}
	};

	/**
	 * Creation of null functions. Note that returning heap allocated objects is not supported yet
	 * (as we can not return null in the general case).
	 */
	template <class R, class A = void, class B = void>
	class NullFn {
	public:
		static FnPtr<R, A, B> *create(Engine &e) {
			return fnPtr(e, &NullFn<R, A, B>::fn);
		}
	private:
		static R CODECALL fn(A a, B b) {
			return CreateObj<R>::create();
		}
	};

	template <class R, class A>
	class NullFn<R, A, void> {
	public:
		static FnPtr<R, A> *create(Engine &e) {
			return fnPtr(e, &NullFn<R, A>::fn);
		}
	private:
		static R CODECALL fn(A a) {
			return CreateObj<R>::create();
		}
	};

	template <class R>
	class NullFn<R, void, void> {
	public:
		static FnPtr<R> *create(Engine &e) {
			return fnPtr(e, &NullFn<R>::fn);
		}
	private:
		static R CODECALL fn() {
			return CreateObj<R>::create();
		}
	};

}
