#pragma once

namespace storm {

	/**
	 * A type handle, ie information about a type without actually knowing
	 * exactly what the type itself is.
	 * Used to make it easier to implement templates usable in Storm from C++.
	 */
	class Handle {
	public:
		// Size of the type.
		const size_t size;

		// Destructor.
		typedef void (*Destroy)(void *);
		const Destroy destroy;

		// Copy (to, from).
		typedef void (*Copy)(void *, const void *);
		const Copy copy;

		// Copy-construct (to, from)
		typedef void (*const Create)(void *, const void *);
		const Create create;
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

		static void destroy(void *v) {
			((T*)v)->~T();
		}

		static void copy(void *to, const void *from) {
			*((T*)to) = *((T*)from);
		}

		static void create(void *to, const void *from) {
			new (to) T(*((T*)from));
		}

	};


	/**
	 * Create handles.
	 */
	template <class T>
	Handle handle() {
		Handle h = {
			HandleHelper<T>::size(),
			&HandleHelper<T>::destroy,
			&HandleHelper<T>::copy,
			&HandleHelper<T>::create,
		};
		return h;
	}

}
