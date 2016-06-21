#pragma once

namespace storm {

	/**
	 * Describes the types declared in C++ which are garbage collected and exposed to Storm.
	 */

	/**
	 * A size. Describes both x86 and x64 sizes.
	 */
	struct CppSize {
		nat size32;
		nat size64;
		nat align32;
		nat align64;

		inline operator Size() const {
			return Size(size32, align32, size64, align64);
		}

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

		inline operator Offset() const {
			return Offset(s32, s64);
		}

		// Invalid mask of (-1, -1). Usable only whenever we expect positive offsets.
		static const CppOffset invalid;
	};

	/**
	 * List of C++ types.
	 */
	struct CppType {
		// Name of the type (null if last element).
		const wchar *name;

		// Parent class' type id.
		nat parent;

		// Total size of the type.
		CppSize size;

		// Pointer offsets. Ends with 'CppSize::invalid'.
		CppOffset *ptrOffsets;
	};


	/**
	 * Contains all information about C++ types and functions required by Storm.
	 */
	struct CppWorld {
		// List of types.
		CppType *types;
	};

	// Get the CppWorld for this module.
	const CppWorld *cppWorld();

}
