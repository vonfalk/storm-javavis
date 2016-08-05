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

		// Parent class' type id.
		nat parent;

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
	 * List of C++ templates.
	 */
	struct CppTemplate {
		// Name of the template (null if last element).
		const wchar *name;

		// Package the template is located inside (eg. a.b.c).
		const wchar *pkg;

		// Generate templates using this function.
		typedef Type *(*GenerateFn)(ValueArray *);
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

		// ...
	};

	/**
	 * Contains all information about C++ types and functions required by Storm.
	 */
	struct CppWorld {
		// List of types.
		CppType *types;

		// List of templates.
		CppTemplate *templates;

		// List of named threads.
		CppThread *threads;
	};

	// Get the CppWorld for this module.
	const CppWorld *cppWorld();

}
