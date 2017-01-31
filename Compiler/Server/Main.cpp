#include "stdafx.h"
#include "Main.h"
#include "Engine.h"
#include "Server.h"
#include "Connection.h"

namespace storm {
	namespace server {


		void run(EnginePtr ep, IStream *input, OStream *output) {
			Engine &e = ep.v;

			Connection *c = new (e) Connection(input, output);

			// Set up standard IO...
			TextOutput *oldOut = e.stdOut();
			TextOutput *oldError = e.stdError();
			e.stdOut(c->textOut);
			e.stdError(c->textOut);

			// TODO: Do something useful with stdin as well!

			// Run the server!
			try {
				Server *s = new (e) Server(c);
				s->run();
			} catch (...) {
				e.stdOut(oldOut);
				e.stdError(oldError);
				throw;
			}

			// Terminate...
			e.stdOut(oldOut);
			e.stdError(oldError);
		}

	}
}
