#pragma once
#include "Core/Hash.h"

namespace sound {

	/**
	 * Encapsulates a Win32 handle so that it can be properly understood by Storm.
	 */
	class Handle {
		STORM_VALUE;
	public:
#ifdef SOUND_DX
		Handle() { value = (size_t)INVALID_HANDLE_VALUE; }
		Handle(HANDLE h) { value = (size_t)h; }

		// Get Win32 types.
		inline HANDLE v() const { return (HANDLE)value; }

#endif

#ifdef SOUND_AL
		Handle() {}
		Handle(ALuint v) { value = v; }

		inline ALuint v() const { return (ALuint)value; }
#endif

		inline Bool STORM_FN operator ==(Handle o) const {
			return value == o.value;
		}

		inline Bool STORM_FN operator !=(Handle o) const {
			return value != o.value;
		}

		inline Nat STORM_FN hash() const {
			return ptrHash((void *)value);
		}

	private:
		UNKNOWN(PTR_NOGC) size_t value;
	};

}
