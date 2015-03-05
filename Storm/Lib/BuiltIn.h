#pragma once
#include "Name.h"

namespace storm {

	class Type;

	/**
	 * A list of all built-in classes.
	 */
	struct BuiltInType {

		// Location (package).
		const wchar *pkg;

		// Name of the type (null in the last element).
		const wchar *name;

		// Static Type pointer index.
		nat typePtrId;

		// Index of super type (id of this entry if none).
		nat super;

		// Size of type.
		nat typeSize;

		// Type flags. (found in TypeFlags)
		int typeFlags;

		// The vtable pointer to the C++ type.
		void *cppVTable;
	};

	/**
	 * Value reference. This can be used to look up an actual value runtime.
	 */
	struct ValueRef {
		// Name of the type.
		const wchar *name;

		// Options (bitmask).
		enum Options {
			nothing = 0x0,

			// Reference to 'name'.
			ref = 0x1,

			// Type is Array<'name'>. If combined with ref, it is Array<'name' &>. Reference
			// to an array (like Array<T> &) is not allowed here.
			array = 0x2,
		};
		Options options;
	};

	inline ValueRef::Options operator &(ValueRef::Options a, ValueRef::Options b) { return ValueRef::Options(int(a)&int(b)); }
	inline ValueRef::Options operator |(ValueRef::Options a, ValueRef::Options b) { return ValueRef::Options(int(a)|int(b)); }

	/**
	 * A list of all built-in functions.
	 */
	struct BuiltInFunction {

		// The location (package).
		const wchar *pkg;

		// Member of a type? null=no.
		const wchar *typeMember;

		// Name of the return type.
		ValueRef result;

		// Name of the function.
		const wchar *name;

		// Parameters to the function (Name of the types).
		vector<ValueRef> params;

		// Function pointer. Null if the last element.
		void *fnPtr;
	};

	/**
	 * A list of all built-in threads.
	 */
	struct BuiltInThread {

		// Location (package).
		const wchar *pkg;

		// Name.
		const wchar *name;

		// Pointer to the thread-declaration type.
		DeclThread *decl;
	};

	/**
	 * Get list of built-in types.
	 */
	const BuiltInType *builtInTypes();

	/**
	 * Get the list. The list ends with a function with a null 'fnPtr'.
	 */
	const BuiltInFunction *builtInFunctions();

	/**
	 * Get the list of built in threads. Ends with a null entry.
	 */
	const BuiltInThread *builtInThreads();

}
