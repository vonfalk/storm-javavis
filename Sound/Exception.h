#pragma once
#include "Core/Exception.h"

namespace sound {

	/**
	 * Error.
	 */
	class EXCEPTION_EXPORT SoundOpenError : public NException {
		STORM_CLASS;
	public:
		SoundOpenError(const wchar *msg) {
			this->msg = new (e) Str(msg);
		}
		STORM_FN SoundOpenError(Str *msg) {
			this->msg = msg;
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};

	/**
	 * Error.
	 */
	class EXCEPTION_EXPORT SoundInitError : public Exception {
	public:
		STORM_CTOR SoundInitError() {}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Failed to initialize sound output.");
		}
	};

}
