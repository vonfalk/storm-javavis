#include "stdafx.h"
#include "Io.h"
#include "Engine.h"

namespace storm {
	namespace io {

		void print(Str *s) {
			stdOut(s->engine())->writeLine(s);
		}

		TextReader *stdIn(EnginePtr e) {
			return e.v.stdIn();
		}

		TextWriter *stdOut(EnginePtr e) {
			return e.v.stdOut();
		}

		TextWriter *stdError(EnginePtr e) {
			return e.v.stdError();
		}

	}
}
