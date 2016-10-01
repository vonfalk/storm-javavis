#pragma once
#include "Core/TypeFlags.h"

namespace storm {

	/**
	 * Describes the types declared in C++ which are garbage collected and exposed to Storm.
	 */

	// Classes which are not always present.
	class ValueArray;
	class Type;

	/**
	 * A size. Describes both x86 and x64 sizes.
	 */
	struct CppSize {
		nat size32;
		nat size64;
		nat align32;
		nat align64;

#ifdef STORM_COMPILER
		// We do not know about code::Size if we're not the compiler.
		inline operator Size() const {
			return Size(size32, align32, size64, align64);
		}
#endif

		static const CppSize invalid;
	};

	/**
	 * An offset. Describes both x86 and x64 sizes.
	 */
	struct CppOffset {
		int s32;
		int s64;

		inline bool operator ==(const CppOffset &o) const {
			return s32 == o.s32 && s64 == o.s64;
		}

		inline bool operator !=(const CppOffset &o) const {
			return !(*this == o);
		}

#ifdef STORM_COMPILER
		// We do not know about code::Offset if we're not the compiler.
		inline operator Offset() const {
			return Offset(s32, s64);
		}
#endif

		// Invalid mask of (-1, -1). Usable only whenever we expect positive offsets.
		static const CppOffset invalid;
	};

	/**
	 * List of C++ types.
	 */
	struct CppType {
		// Name of the type (null if last element).
		const wchar *name;

		// Package the type is located inside (eg. a.b.c).
		const wchar *pkg;

		// What kind of parent do we have?
		enum SuperKind {
			// No parent.
			superNone,

			// 'super' is a class in this table.
			superClass,

			// 'super' is a thread id (we inherit from TObject).
			superThread,

			// 'super' is a pointer to a function CreateFn that generates the type to use.
			superCustom,
		};

		// Parent kind.
		SuperKind kind;

		// The create function that 'super' is if 'kind == superCustom'.
		typedef Type *(*CreateFn)(Str *name, Size size, GcType *type);

		// Super class' type id, see 'kind' for the exact meaning.
		size_t super;

		// Total size of the type.
		CppSize size;

		// Pointer offsets. Ends with 'CppSize::invalid'.
		CppOffset *ptrOffsets;

		// Type flags (final? value type? etc.)
		TypeFlags flags;

		// Destructor to use for this type (if any).
		const void *destructor;
	};

	/**
	 * Reference to a type from C++.
	 */
	struct CppFnRef {
		// Type or template index. If 'params' is null, this is a type index, otherwise it is a
		// template index and 'params' indicates the parameters to that template.
		size_t id;

		// If we're a template, this array is populated. It ends with size_t(-1).
		size_t *params;

		// Is this a maybe-type?
		bool maybe;
	};

	/**
	 * List of C++ functions.
	 */
	struct CppFunction {
		// Name of the function (null if last element).
		const wchar *name;

		// Package. This is 'null' if this is a member function.
		const wchar *pkg;

		// Kind of function.
		enum FnKind {
			// Free function. Nothing strange.
			fnFree,

			// Free function, but a EnginePtr is to be inserted as the first parameter.
			fnFreeEngine,

			// Member function. The first parameter indicates the type we're a member of. 'pkg' is null.
			fnMember,
		};

		// Kind.
		FnKind kind;

		// Thread to run this function on. Only relevant if this function is not a member function.
		size_t threadId;

		// Pointer to the function.
		const void *ptr;

		// Parameters.
		CppFnRef *params;

		// Result.
		CppFnRef result;
	};

	/**
	 * List of C++ templates.
	 */
	struct CppTemplate {
		// Name of the template (null if last element).
		const wchar *name;

		// Package the template is located inside (eg. a.b.c).
		const wchar *pkg;

		// Generate templates using this function.
		typedef Type *(*GenerateFn)(Str *name, ValueArray *);
		GenerateFn generate;
	};

	/**
	 * List of C++ named threads.
	 */
	struct CppThread {
		// Name of the thread (null if last element).
		const wchar *name;

		// Package the thread is located inside.
		const wchar *pkg;

		// Thread declaration.
		DeclThread *decl;

		// ...
	};

	/**
	 * Contains all information about C++ types and functions required by Storm.
	 */
	struct CppWorld {
		// List of types.
		CppType *types;

		// List of functions.
		CppFunction *functions;

		// List of templates.
		CppTemplate *templates;

		// List of named threads.
		CppThread *threads;
	};

	// Get the CppWorld for this module.
	const CppWorld *cppWorld();

}
