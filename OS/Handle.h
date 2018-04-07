#pragma once

namespace os {

	/**
	 * Opaque wrapper of a system dependent OS handle to avoid mistakingly casting the handle to an
	 * erroneous type. This has to be pointer-sized.
	 */
#ifdef WINDOWS
	class Handle {
	public:
		inline Handle() : z(INVALID_HANDLE_VALUE) {}
		inline Handle(HANDLE v) : z(v) {}
		inline HANDLE v() const { return z; }
		inline operator bool () const { return z != INVALID_HANDLE_VALUE; }
	private:
		HANDLE z;
	};
#else
	class Handle {
	public:
		inline Handle() : z(-1) {}
		inline Handle(int v) : z(v) {}
		inline int v() const { return (int)this->z; }
		inline operator bool () const { return v() >= 0; }
	private:
		ptrdiff_t z;
	};
#endif

}
