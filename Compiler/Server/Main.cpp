#include "stdafx.h"
#include "Main.h"
#include "Engine.h"
#include "SExpr.h"
#include "Connection.h"
#include "Core/Io/MemStream.h"

namespace storm {
	namespace server {


		void run(EnginePtr ep, IStream *input, OStream *output) {
			Engine &e = ep.v;

			Connection *c = new (e) Connection(input, output);

			// Set up standard IO...
			TextWriter *oldOut = e.stdOut();
			TextWriter *oldError = e.stdError();
			e.stdOut(c->textOut);
			e.stdError(c->textOut);

			// TODO: Do something useful with stdin as well?

			// Start up!
			TextWriter *out = c->textOut;
			out->writeLine(new (e) Str(L"Welcome to the language server!"));

			SExpr *msg = list(e, 4,
							new (e) Number(1),
							new (e) String(L"string"),
							list(e, 2,
								new (e) Number(2),
								new (e) Number(3)),
							c->symbol(L"storm"));
			c->send(msg);

			Sleep(1000);


			// Terminate...
			e.stdOut(oldOut);
			e.stdError(oldError);
		}

	}
}
