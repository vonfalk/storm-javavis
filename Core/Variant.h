#pragma once

namespace storm {
	STORM_PKG(core);

	/**
	 * A class that represents an instance of some type in the system. Similar to "void *" in C.
	 *
	 * The interface to this class in Storm is not yet complete; this class was created as a useful
	 * tool for implementing serialization and other generic facilities in C++.
	 */
	class Variant {
		STORM_VALUE;
	public:
		// Create an empty variant.
		STORM_CTOR Variant();

		// Copy the variant.
		Variant(const Variant &o);
		Variant &operator =(const Variant &o);

		// Create a variant referring to an object.
		explicit Variant(RootObject *t);

		// Create a variant referring to a value.
		template <class T>
		Variant(const T &v, Engine &e) {
			init(&v, StormInfo<T>::type(e));
		}

		// Create a variant referring to some known type known to be a value.
		Variant(const void *value, Type *type);

		// Destroy. Call the destructor of the contained element, if any.
		~Variant();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Empty/any value?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const { return !empty(); }

		// Does this variant contain the specified type?
		Bool STORM_FN has(Type *type) const;

	public:

		/**
		 * Low-level API for C++.
		 */

		// Create a uninitialized variant referring to a type. Call 'getValue' and 'valueInitialized'
		// to initialize the value.
		static Variant uninitializedValue(Type *type);

		// Get a pointer to the value stored in here, assuming it is a value.
		void *getValue();

		// Note that we have initialized the value.
		void valueInitialized();

		// Note that we have moved the value somewhere else, clearing the variant without calling
		// the destructor. It is possible to call 'valueInitialized' again.
		void valueRemoved();

		// Move the value somewhere else.
		void moveValue(void *to);

		// Get the object stored in here.
		RootObject *getObject();

	private:
		// The stored data. Either an array with one element, or a pointer to an object.
		UNKNOWN(PTR_GC) void *data;

		// Initialize for a value.
		void init(const void *value, Type *type);

		// Find an Engine instance. Assumes we're not empty.
		Engine &engine() const;

		friend StrBuf &operator <<(StrBuf &to, const Variant &v);
		friend wostream &operator <<(wostream &to, const Variant &v);
	};

	// Output.
	StrBuf &STORM_FN operator <<(StrBuf &to, const Variant &v);
	wostream &operator <<(wostream &to, const Variant &v);

}
