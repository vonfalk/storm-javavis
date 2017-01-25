#include "stdafx.h"
#include "Connection.h"
#include "Core/Io/Utf8Text.h"
#include "Core/Io/MemStream.h"

namespace storm {
	namespace server {

		Connection::Connection(IStream *input, OStream *output)
			: input(input), output(output) {

			TextInfo info;
			info.useCrLf = false;
			info.useBom = false;
			textOut = new (this) Utf8Writer(output, info);
			symNames = new (this) NameMap();
			symIds = new (this) IdMap();
			lastSymId = 0x80000000; // Emacs only uses ~30 bits for integers.
		}

		Symbol *Connection::symbol(const wchar *name) {
			return symbol(new (this) Str(name));
		}

		Symbol *Connection::symbol(Str *name) {
			NameMap::Iter i = symNames->find(name);
			if (i == symNames->end()) {
				Symbol *sym = new (this) Symbol(name, --lastSymId);
				symNames->put(name, sym);
				return sym;
			} else {
				return i.v();
			}
		}

		Symbol *Connection::symbol(Nat id) {
			IdMap::Iter i = symIds->find(id);
			if (i == symIds->end()) {
				return null;
			} else {
				return i.v();
			}
		}

		Bool Connection::sendSymbol(Symbol *sym) {
			if (symIds->has(sym->id))
				return false;

			symIds->put(sym->id, sym);
			return true;
		}

		void Connection::send(SExpr *expr) {
			OMemStream *out = new (this) OMemStream();

			// Leading zero byte.
			GcPreArray<byte, 1> d;
			d.v[0] = 0x00;
			out->write(Buffer(d));

			// Rest of the message.
			serialize(out, expr);

			// Send the message.
			textOut->writeLine(expr->toS());
			textOut->writeLine(out->toS());
			output->write(out->buffer());
		}

		void Connection::serialize(OStream *to, MAYBE(SExpr *) expr) {
			if (expr) {
				expr->serialize(to, this);
			} else {
				GcPreArray<byte, 1> d;
				d.v[0] = 0x00;
				to->write(Buffer(d));
			}
		}

	}
}
