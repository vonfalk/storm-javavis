#pragma once

namespace storm {

	class Type;

	/**
	 * The root object that all non-value objects inherit
	 * from. This class contains the logic for the central
	 * reference counting mechanism among other things.
	 * These are designed to be manipulated through pointer,
	 * and is therefore not copyable using regular C++ methods.
	 */
	class Object : NoCopy {
	public:
		// Initialize object to 0 references.
		Object(Type *type);

		virtual ~Object();

		// The type of this object.
		Type *const type;

		// Add reference.
		inline void addRef() {
			atomicIncrement(refs);
		}

		// Release reference.
		inline void release() {
			if (atomicDecrement(refs) == 0)
				delete this;
		}

	private:
		// Current number of references.
		nat refs;

		// We shall use release() instead of delete.
		void operator delete(void *ptr);
		void operator delete[](void *ptr);
	};

}
