#include "stdafx.h"
#include "Io.h"
#include "Engine.h"

namespace storm {
	namespace io {

		void print(Str *s) {
			stdOut(s->engine())->writeLine(s);
		}

		TextInput *stdIn(EnginePtr e) {
			return e.v.stdIn();
		}

		TextOutput *stdOut(EnginePtr e) {
			return e.v.stdOut();
		}

		TextOutput *stdError(EnginePtr e) {
			return e.v.stdError();
		}

	}
}
