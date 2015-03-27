#pragma once
#include "Code/Reference.h"

CREATE_DETECTOR(deepCopy);

namespace storm {
	class CloneEnv;

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
		typedef void (CODECALL *Create)(void *, const void *);
		Create create;

		// Deep-copy.
		typedef void (CODECALL *DeepCopy)(void *, CloneEnv *);
		DeepCopy deepCopy;

		inline Handle()
			: size(0), destroy(null), create(null), deepCopy(null) {}
		inline Handle(size_t size, Destroy destroy, Create create, DeepCopy deep)
			: size(size), destroy(destroy), create(create), deepCopy(deep) {}
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
		void deepCopyRef(const code::Ref &ref);
		void deepCopyRef();
		void createRef(const code::Ref &ref);

	private:
		// References.
		code::AddrReference *destroyUpdater;
		code::AddrReference *createUpdater;
		code::AddrReference *deepCopyUpdater;
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

	template <class T, bool hasDeepCopy>
	class HandleDeepHelper {
	public:
		static Handle::DeepCopy deepCopy() { return null; }
	};

	template <class T>
	class HandleDeepHelper<T, true> {
	public:
		static Handle::DeepCopy deepCopy() {
			return (Handle::DeepCopy)address(&T::deepCopy);
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
			&HandleHelper<T>::create,
			HandleDeepHelper<T, detect_deepCopy<T>::value>::deepCopy()
			);
		return h;
	}

}
