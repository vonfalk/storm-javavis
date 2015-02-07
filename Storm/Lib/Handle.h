#pragma once
#include "Code/Reference.h"

namespace storm {

	/**
	 * A type handle, ie information about a type without actually knowing
	 * exactly what the type itself is.
	 * Used to make it easier to implement templates usable in Storm from C++.
	 *
	 * NOTE: Do not copy, introducing NoCopy here crashes the static variable below
	 * for some reason...
	 */
	class Handle {
	public:
		// Size of the type.
		size_t size;

		// Destructor (may be null).
		typedef void (CODECALL *Destroy)(void *);
		Destroy destroy;

		// Copy-construct (to, from)
		typedef void (CODECALL *const Create)(void *, const void *);
		Create create;

		inline Handle()
			: size(0), destroy(null), create(null) {}
		inline Handle(size_t size, Destroy destroy, Create create)
			: size(size), destroy(destroy), create(create) {}
	};

	/**
	 * Use a handle which updates automagically.
	 */
	class RefHandle : public Handle {
	public:
		// Ctor.
		RefHandle();

		// Dtor.
		~RefHandle();

		// Set references.
		void destroyRef(const code::Ref &ref);
		void destroyRef();
		void createRef(const code::Ref &ref);

	private:
		// References.
		code::AddrReference *destroyUpdater;
		code::AddrReference *createUpdater;
	};

	/**
	 * Helper, this allows us to specialize for various types later on!
	 */
	template <class T>
	class HandleHelper {
	public:
		static size_t size() {
			return sizeof(T);
		}

		static void CODECALL destroy(void *v) {
			((T*)v)->~T();
		}

		static void CODECALL create(void *to, const void *from) {
			new (to) T(*((T*)from));
		}

	};


	/**
	 * Create handles.
	 */
	template <class T>
	const Handle &handle() {
		static Handle h = Handle(
			HandleHelper<T>::size(),
			&HandleHelper<T>::destroy,
			&HandleHelper<T>::create
			);
		return h;
	}

}
