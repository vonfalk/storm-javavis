#include "stdafx.h"
#include "Connection.h"
#include "Core/Io/Utf8Text.h"
#include "Core/Io/MemStream.h"

namespace storm {
	namespace server {

		ReadResult::ReadResult() : consumed(0), result(0) {}

		ReadResult::ReadResult(Nat c, MAYBE(SExpr *) e) : consumed(c), result(e) {}


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
			out->write(fullBuffer(d));

			// Rest of the message.
			write(out, expr);

			// Send the message.
			textOut->writeLine(expr->toS());
			textOut->writeLine(out->toS());
			output->write(out->buffer());
		}

		MAYBE(SExpr *) Connection::receive() {
			return null;
		}

		void Connection::write(OStream *to, MAYBE(SExpr *) expr) {
			if (expr) {
				expr->write(to, this);
			} else {
				GcPreArray<byte, 1> d;
				d.v[0] = 0x00;
				to->write(fullBuffer(d));
			}
		}

		ReadResult Connection::read(IStream *from) {
			GcPreArray<byte, 1> type;
			Buffer r = from->read(emptyBuffer(type));
			if (r.filled() == 0)
				return ReadResult();

			switch (r[0]) {
			case SExpr::nil:
				return ReadResult(1, null);
			case SExpr::cons:
				return readCons(from) + 1;
			case SExpr::number:
				return readNumber(from) + 1;
			case SExpr::string:
				return readString(from) + 1;
			case SExpr::newSymbol:
				return readNewSymbol(from) + 1;
			case SExpr::oldSymbol:
				return readOldSymbol(from) + 1;
			default:
				// Return null for this sub-expression, so that parsing goes on...
				return ReadResult(1, null);
			}
		}

		ReadResult Connection::readCons(IStream *from) {
			ReadResult first = read(from);
			if (first.failed())
				return first;
			ReadResult rest = read(from);
			if (rest.failed())
				return rest;
			return ReadResult(first.consumed + rest.consumed,
							new (this) Cons(first.result, rest.result));
		}

		static Nat decodeNat(Buffer r) {
			return (Nat(r[0]) << 24)
				| (Nat(r[1]) << 16)
				| (Nat(r[2]) << 8)
				| (Nat(r[3]) << 0);
		}

		ReadResult Connection::readNumber(IStream *from) {
			GcPreArray<byte, 4> buf;
			Buffer r = from->read(emptyBuffer(buf));
			if (r.filled() < 4)
				return ReadResult();

			Nat v = decodeNat(r);
			return ReadResult(4, new (this) Number(Int(v)));
		}

		ReadResult Connection::readString(IStream *from) {
			GcPreArray<byte, 4> buf;
			Buffer r = from->read(emptyBuffer(buf));
			if (r.filled() < 4)
				return ReadResult();

			Nat len = decodeNat(r);
			Buffer str = from->read(len);
			if (str.filled() < len)
				return ReadResult();

			IMemStream *src = new (this) IMemStream(str);
			TextReader *text = new (this) Utf8Reader(src);
			Str *data = text->readAllRaw();

			return ReadResult(len + 4, new (this) String(data));
		}

		ReadResult Connection::readNewSymbol(IStream *from) {
			GcPreArray<byte, 4> buf;
			Buffer r = from->read(emptyBuffer(buf));
			if (r.filled() < 4)
				return ReadResult();
			Nat symId = decodeNat(r);

			r = from->read(emptyBuffer(buf));
			if (r.filled() < 4)
				return ReadResult();
			Nat len = decodeNat(r);

			Buffer data = from->read(len);
			if (data.filled() < len)
				return ReadResult();

			IMemStream *src = new (this) IMemStream(data);
			TextReader *text = new (this) Utf8Reader(src);
			Str *name = text->readAllRaw();

			// Insert the symbol if it does not already exist.
			Symbol *sym = null;
			if (symNames->has(name)) {
				sym = symNames->get(name);
			} else {
				sym = new (this) Symbol(name, symId);
				symNames->put(name, sym);
			}

			symIds->put(symId, sym);
			return ReadResult(len + 8, sym);
		}

		ReadResult Connection::readOldSymbol(IStream *from) {
			GcPreArray<byte, 4> buf;
			Buffer r = from->read(emptyBuffer(buf));
			if (r.filled() < 4)
				return ReadResult();
			Nat symId = decodeNat(r);

			Symbol *sym = symIds->get(symId, null);
			// Return 'sym' even if it is null, so that we can proceed reading input after the
			// corrupt data.
			return ReadResult(4, sym);
		}

	}
}
