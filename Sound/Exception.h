#pragma once
#include "Core/Exception.h"

namespace sound {

	/**
	 * Error.
	 */
	class EXCEPTION_EXPORT SoundOpenError : public storm::Exception {
		STORM_EXCEPTION;
	public:
		SoundOpenError(const wchar *msg) {
			this->msg = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR SoundOpenError(Str *msg) {
			this->msg = msg;
			saveTrace();
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
	class EXCEPTION_EXPORT SoundInitError : public storm::Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR SoundInitError() {
			saveTrace();
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Failed to initialize sound output.");
		}
	};

}
