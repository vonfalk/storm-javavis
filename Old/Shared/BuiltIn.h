#pragma once
#include "Utils/Bitmask.h"

namespace storm {

	class Type;

	/**
	 * A list of all built-in classes.
	 */
	struct BuiltInType {

		// Meaning of the 'super' field.
		enum SuperMode {
			// No super class.
			superNone,
			// Regular super class.
			superClass,
			// Hidden super class.
			superHidden,
			// Reference to a thread (these objects always inherit from TObject anyway).
			superThread,
			// Import the type from another place. This means that 'pkg' is never relative.
			// 'super' is not used in this case.
			superExternal,
		};

		// Location (package). Relative to the package root in case of an external library, otherwise absolute.
		const wchar *pkg;

		// Name of the type (null in the last element).
		const wchar *name;

		// Static Type pointer index.
		nat typePtrId;

		// Index of super type. See 'superField' for the meaning.
		nat super;

		// Mode of the 'super' field.
		SuperMode superMode;

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
	struct ValueInnerRef {
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

			// Type is a function pointer. This means that this is an instance of 'ValueRef'.
			// Function pointers are "inside" of arrays, which means that if 'array' and 'fnPtr' are
			// set, the type is Array<FnPtr<...>>
			fnPtr = 0x4,

			// Type is a 'maybe' pointer. Always outermost, Array<Maybe<T>> is not supported.
			maybe = 0x8,
		};
		Options options;
	};

	BITMASK_OPERATORS(ValueInnerRef::Options);

	/**
	 * Value reference, also supports function pointers. Note that this structure does not support
	 * functions taking functions as parameters, even though the Storm type system does that. This
	 * situation is quite rare and not present in the C++ code anyway, so ignoring that special case
	 * makes the structure static and much simpler to deal with.
	 */
	struct ValueRef : ValueInnerRef {
		// Max # of function parameters.
		static const nat maxParams = 2;

		// Result of function pointer.
		ValueInnerRef result;

		// Parameters of function pointer.
		ValueInnerRef params[2];
	};


	/**
	 * A list of all built-in functions.
	 */
	struct BuiltInFunction {

		// Flags.
		enum Mode {
			// Not a member.
			noMember = 0x0001,

			// Member of a type.
			typeMember = 0x0002,

			// Run on a thread.
			onThread = 0x0010,

			// The function takes a 'engine' as the firt parameter, not visible to storm.
			hiddenEngine = 0x0020,

			// This function is declared virtual.
			virtualFunction = 0x0040,

			// This function is declared as a setter.
			setterFunction = 0x0080,

			// This function is declared as an auto-cast constructor.
			castCtor = 0x0100,
		};

		// Mode.
		Mode mode;

		// The location (package). Only valid if 'noMember' is set.
		const wchar *pkg;

		// Member of a type? Only valid if 'typeMember' is set.
		nat memberId;

		// Name of the return type.
		ValueRef result;

		// Name of the function.
		const wchar *name;

		// Parameters to the function (Name of the types).
		vector<ValueRef> params;

		// Function pointer. Null if the last element.
		void *fnPtr;

		// Run on a specific thread. Only valid if 'onThread' is set in 'mode'.
		nat threadId;
	};

	BITMASK_OPERATORS(BuiltInFunction::Mode);

	/**
	 * A list of all built-in variables (of types).
	 */
	struct BuiltInVar {
		// Member of a type.
		nat memberId;

		// Type.
		ValueRef type;

		// Name of the variable.
		const wchar *name;

		// Offset of the variable. TODO: Offset for different platforms!
		size_t offset;
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
	 * Get the list of variables. The list ends with an entry containing 'name' == null.
	 */
	const BuiltInVar *builtInVars();

	/**
	 * Get the list of built in threads. Ends with a null entry.
	 */
	const BuiltInThread *builtInThreads();

	/**
	 * Struct holding all built-in things.
	 */
	struct BuiltIn {
		const BuiltInType *types;
		const BuiltInFunction *functions;
		const BuiltInVar *variables;
		const BuiltInThread *threads;

		// The VTable for our version of Object (to make toS work well).
		const void *objectVTable;
	};

	/**
	 * Get all built-in things from here.
	 */
	const BuiltIn &builtIn();

}