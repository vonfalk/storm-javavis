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
			inputBuffer = new (this) OMemStream();
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
			// textOut->writeLine(expr->toS());
			// textOut->writeLine(out->toS());
			output->write(out->buffer());
		}

		SExpr *Connection::receive() {
			// Read messages from the buffer until we fail (all failures are due to not enough data).
			SExpr *result = null;
			while (result == null) {
				ReadResult r = readBuffer();
				if (r.failed()) {
					// We need to read more data!
					if (!fillBuffer())
						return null;
				} else {
					// Remove data from the buffer.
					Buffer trimmed = cut(engine(), inputBuffer->buffer(), r.consumed);
					inputBuffer = new (this) OMemStream(trimmed);
					result = r.result;
				}
			}

			return result;
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

		static Nat bufLen(const Buffer &b) {
			for (Nat i = 0; i < b.filled(); i++)
				if (b[i] == 0x00)
					return i;

			return b.filled();
		}

		ReadResult Connection::readBuffer() {
			Buffer b = inputBuffer->buffer();
			if (b.empty())
				return ReadResult();

			// An SExpr or a raw string at the front?
			if (b[0] == 0x00) {
				// textOut->writeLine(new (this) Str(L"Trying to parse:"));
				// textOut->writeLine((new (this) IMemStream(b, 1))->toS());

				// Message, try to parse it!
				ReadResult r = read(new (this) IMemStream(b, 1));
				if (!r.failed())
					r = r + 1;

				// textOut->writeLine(new (this) Str((L"Consumed " + ::toS(r.consumed)).c_str()));
				return r;
			} else {
				// A plain string. Forward it to our stdin.
				Nat len = bufLen(b);
				// TODO: We do not maintain a stdin yet. For now: just throw the data away!
				return ReadResult(len, null);
			}
		}

		Bool Connection::fillBuffer() {
			const Nat chunk = 1024;
			Buffer r = input->read(chunk);
			if (r.empty())
				return false;

			inputBuffer->write(r);
			return true;
		}

		ReadResult Connection::read(IStream *from) {
			GcPreArray<byte, 1> type;
			Buffer kind = from->read(emptyBuffer(type));
			if (kind.filled() == 0)
				return ReadResult();

			ReadResult r;
			switch (kind[0]) {
			case SExpr::nil:
				r = ReadResult(1, null);
				break;
			case SExpr::cons:
				r = readCons(from);
				break;
			case SExpr::number:
				r = readNumber(from);
				break;
			case SExpr::string:
				r = readString(from);
				break;
			case SExpr::newSymbol:
				r = readNewSymbol(from);
				break;
			case SExpr::oldSymbol:
				r = readOldSymbol(from);
				break;
			default:
				// Return null for this sub-expression, so that parsing goes on...
				textOut->writeLine(new (this) Str(L"WARNING: Invalid type found in message."));
				return ReadResult(1, null);
			}

			if (!r.failed())
				r = r + 1;
			return r;
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
