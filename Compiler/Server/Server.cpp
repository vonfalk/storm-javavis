#include "stdafx.h"
#include "Server.h"

namespace storm {
	namespace server {

		Server::Server(Connection *c) : conn(c) {
			quit = c->symbol(L"quit");
		}

		void Server::run() {
			print(L"Language server started.");

			SExpr *msg = null;
			while (msg = conn->receive()) {
				if (!process(msg))
					break;
			}

			print(L"Terminating. Bye!");
		}

		Bool Server::process(SExpr *msg) {
			print(L"Processing message: " + ::toS(msg));
			Cons *cell = as<Cons>(msg);
			if (!cell)
				return true;

			Symbol *kind = as<Symbol>(cell->first);
			if (!kind)
				return true;

			if (quit->equals(kind)) {
				return false;
			}

			return true;
		}

		void Server::print(Str *s) {
			conn->textOut->writeLine(s);
		}

		void Server::print(const wchar *s) {
			print(new (this) Str(s));
		}

		void Server::print(const CString &s) {
			print(new (this) Str(s.c_str()));
		}

	}
}
