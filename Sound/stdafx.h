#pragma once

#include "Utils/Utils.h"
#include "Shared/Storm.h"

#include "Utils/Windows.h"

#include "Shared/DllEngine.h"

using namespace storm;


namespace storm {

	/**
	 * Global data.
	 */
	class LibData : public NoCopy {
	public:
		LibData();
		~LibData();

		// Nothing here yet.
	};

}

namespace sound {
	typedef storm::LibData LibData;

    /**
	 * Error.
	 */
	class SoundOpenError : public Exception {
	public:
		SoundOpenError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Error while loading sound: " + msg; }
	private:
		String msg;
	};
}


#include "Shared/Io/Stream.h"
