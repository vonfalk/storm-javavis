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
		Bool STORM_FN empty() const { return data == null; }
		Bool STORM_FN any() const { return data != null; }

		// Does this variant contain the specified type?
		Bool STORM_FN has(Type *type) const;

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
