#pragma once

namespace os {

	/**
	 * Base class for objects that the OS library recognizes as being thrown by pointer.
	 *
	 * Objects inheriting from this class, and thrown by pointer are treated differently by the
	 * implementation so that such pointers can be exposed to a garbage collector properly.
	 */
	class PtrThrowable {
	public:
		virtual ~PtrThrowable() {}

		// Get a string representation.
		virtual const wchar *toCStr() const = 0;
	};

}
