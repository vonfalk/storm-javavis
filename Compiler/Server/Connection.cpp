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
				debug = false;
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
				// Message, try to parse it!
				IMemStream *stream = new (this) IMemStream(b);
				stream->seek(1);

				Bool ok = true;
				SExpr *result = read(stream, ok);

				// textOut->writeLine(new (this) Str(L"Tried parsing:"));
				// textOut->writeLine(stream->toS());

				if (!ok) {
					textOut->writeLine(new (this) Str(L"Failed:"));
					textOut->writeLine(stream->toS());
					return ReadResult();
				} else {
					if (!as<Cons>(result)) {
						textOut->writeLine(new (this) Str(L"This seems bad...."));
						textOut->writeLine(stream->toS());
					}
					// textOut->writeLine(new (this) Str((L"Consumed " + ::toS(stream->tell())).c_str()));
					return ReadResult(Nat(stream->tell()), result);
				}
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

			textOut->writeLine(new (this) Str(L"Old data:"));
			textOut->writeLine(inputBuffer->toS());
			inputBuffer->write(r);
			textOut->writeLine(new (this) Str(L"Reading data..."));
			textOut->writeLine((new (this) IMemStream(r))->toS());
			debug = true;
			return true;
		}

		static Byte decodeByte(IStream *src, Bool &ok) {
			GcPreArray<byte, 1> buf;
			Buffer r = src->read(emptyBuffer(buf));

			if (!r.full()) {
				ok = false;
				return 0;
			}

			return r[0];
		}

		static Byte peekByte(IStream *src, Bool &ok) {
			GcPreArray<byte, 1> buf;
			Buffer r = src->peek(emptyBuffer(buf));

			if (!r.full()) {
				ok = false;
				return 0;
			}

			return r[0];
		}

		static Nat decodeNat(IStream *src, Bool &ok) {
			GcPreArray<byte, 4> buf;
			Buffer r = src->read(emptyBuffer(buf));

			if (!r.full()) {
				ok = false;
				return 0;
			}

			return (Nat(r[0]) << 24)
				| (Nat(r[1]) << 16)
				| (Nat(r[2]) << 8)
				| (Nat(r[3]) << 0);
		}


		MAYBE(SExpr *) Connection::read(IStream *from, Bool &ok) {
			if (!ok)
				return null;

			Byte kind = decodeByte(from, ok);
			switch (kind) {
			case SExpr::nil:
				return null;
			case SExpr::cons:
				return readCons(from, ok);
			case SExpr::number:
				return readNumber(from, ok);
			case SExpr::string:
				return readString(from, ok);
			case SExpr::newSymbol:
				return readNewSymbol(from, ok);
			case SExpr::oldSymbol:
				return readOldSymbol(from, ok);
			default:
				// Return null for this sub-expression, so that parsing goes on...
				textOut->writeLine(new (this) Str(L"WARNING: Invalid type found in message."));
				return null;
			}
		}

		MAYBE(SExpr *) Connection::readCons(IStream *from, Bool &ok) {
			Cons *first = new (this) Cons(read(from, ok), null);
			Cons *curr = first;
			while (ok && peekByte(from, ok) == SExpr::cons) {
				// Consume the byte we peeked.
				decodeByte(from, ok);

				Cons *t = new (this) Cons(read(from, ok), null);
				curr->rest = t;
				curr = t;
			}

			if (!ok)
				return null;

			// Read the last cell as well.
			curr->rest = read(from, ok);
			return first;
		}

		MAYBE(SExpr *) Connection::readNumber(IStream *from, Bool &ok) {
			Nat r = decodeNat(from, ok);
			if (!ok)
				return null;

			return new (this) Number(Int(r));
		}

		MAYBE(SExpr *) Connection::readString(IStream *from, Bool &ok) {
			Nat len = decodeNat(from, ok);
			if (!ok)
				return null;

			Buffer str = from->read(len);
			if (str.filled() < len) {
				ok = false;
				return null;
			}

			IMemStream *src = new (this) IMemStream(str);
			TextReader *text = new (this) Utf8Reader(src);
			Str *data = text->readAllRaw();

			return new (this) String(data);
		}

		MAYBE(SExpr *) Connection::readNewSymbol(IStream *from, Bool &ok) {
			Nat symId = decodeNat(from, ok);
			Nat len = decodeNat(from, ok);
			if (!ok)
				return null;

			Buffer str = from->read(len);
			if (str.filled() < len) {
				ok = false;
				return null;
			}

			IMemStream *src = new (this) IMemStream(str);
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

			return sym;
		}

		MAYBE(SExpr *) Connection::readOldSymbol(IStream *from, Bool &ok) {
			Nat symId = decodeNat(from, ok);
			if (!ok)
				return null;

			Symbol *sym = symIds->get(symId, null);
			// Return 'sym' even if it is null, so that we can proceed reading input after the
			// corrupt data.
			return sym;
		}

	}
}
