#include "stdafx.h"
#include "SExpr.h"
#include "Connection.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/Hash.h"
#include "Core/CloneEnv.h"
#include "Core/Io/MemStream.h"
#include "Core/Io/Utf8Text.h"

namespace storm {
	namespace server {

		static void writeInt(OStream *to, Int v) {
			GcPreArray<byte, 4> d;
			d.v[0] = byte((Nat(v) >> 24) & 0xFF);
			d.v[1] = byte((Nat(v) >> 16) & 0xFF);
			d.v[2] = byte((Nat(v) >>  8) & 0xFF);
			d.v[3] = byte((Nat(v) >>  0) & 0xFF);
			to->write(Buffer(d));
		}

		static void writeInt(OStream *to, byte header, Int v) {
			GcPreArray<byte, 5> d;
			d.v[0] = header;
			d.v[1] = byte((Nat(v) >> 24) & 0xFF);
			d.v[2] = byte((Nat(v) >> 16) & 0xFF);
			d.v[3] = byte((Nat(v) >>  8) & 0xFF);
			d.v[4] = byte((Nat(v) >>  0) & 0xFF);
			to->write(Buffer(d));
		}

		SExpr::SExpr() {}

		void SExpr::serialize(OStream *to, Connection *c) {
			// Serialize as 'null'.
			GcPreArray<byte, 1> d;
			d.v[0] = 0x00;
			to->write(Buffer(d));
		}


		Cons::Cons() {}

		Cons::Cons(MAYBE(SExpr *) first, MAYBE(SExpr *) rest)
			: first(first), rest(rest) {}

		void Cons::deepCopy(CloneEnv *env) {
			cloned(first, env);
			cloned(rest, env);
		}

		void Cons::toS(StrBuf *to) const {
			*to << L"(" << first;

			SExpr *at = rest;
			while (at) {
				Cons *n = as<Cons>(at);
				if (n) {
					*to << L" " << n->first;
					at = n->rest;
				} else {
					*to << L" . " << at;
				}
			}

			*to << L")";
		}

		void Cons::serialize(OStream *to, Connection *c) {
			GcPreArray<byte, 1> d;
			d.v[0] = 0x01;
			to->write(Buffer(d));

			c->serialize(to, first);
			c->serialize(to, rest);
		}

		Cons *cons(EnginePtr e, MAYBE(SExpr *) first, MAYBE(SExpr *) rest) {
			return new (e.v) Cons(first, rest);
		}

		Cons *list(Array<SExpr *> *src) {
			if (src->empty())
				return null;

			Engine &e = src->engine();
			Cons *first = new (e) Cons(src->at(0), null);
			Cons *now = first;
			for (nat i = 1; i < src->count(); i++) {
				Cons *t = new (e) Cons(src->at(i), null);
				now->rest = t;
				now = t;
			}

			return first;
		}

		Cons *list(Engine &e, Nat count, ...) {
			if (count == 0)
				return null;

			va_list l;
			va_start(l, count);

			Cons *first = new (e) Cons(va_arg(l, SExpr *), null);
			Cons *now = first;
			for (nat i = 1; i < count; i++) {
				Cons *t = new (e) Cons(va_arg(l, SExpr *), null);
				now->rest = t;
				now = t;
			}

			va_end(l);
			return first;
		}

		Number::Number(Int v) : v(v) {}

		void Number::deepCopy(CloneEnv *env) {
			cloned(v, env);
		}

		void Number::toS(StrBuf *to) const {
			*to << v;
		}

		void Number::serialize(OStream *to, Connection *c) {
			writeInt(to, 0x02, v);
		}

		String::String(const wchar *str) {
			v = new (this) Str(str);
		}

		String::String(Str *v) : v(v) {}

		void String::deepCopy(CloneEnv *env) {
			cloned(v, env);
		}

		void String::toS(StrBuf *to) const {
			*to << L"\"" << v->escape() << L"\"";
		}

		void String::serialize(OStream *to, Connection *c) {
			OMemStream *mem = new (this) OMemStream();
			TextWriter *text = new (this) Utf8Writer(mem);
			text->write(v);
			text->flush();

			Buffer strData = mem->buffer();
			writeInt(to, 0x03, strData.filled());
			to->write(strData);
		}


		Symbol::Symbol(Str *v, Nat id) : v(v), id(id) {}

		void Symbol::deepCopy(CloneEnv *env) {
			// No need, we're immutable!
		}

		void Symbol::toS(StrBuf *to) const {
			*to << L"'" << v->escape(' ');
		}

		Bool Symbol::equals(Object *o) const {
			if (Object::equals(o))
				return id == ((Symbol *)o)->id;
			return false;
		}

		Nat Symbol::hash() const {
			return natHash(id);
		}

		void Symbol::serialize(OStream *to, Connection *c) {
			if (c->sendSymbol(this)) {
				// Fist time, send the entire symbol!
				writeInt(to, 0x04, id);

				OMemStream *mem = new (this) OMemStream();
				TextWriter *text = new (this) Utf8Writer(mem);
				text->write(v);
				text->flush();

				Buffer strData = mem->buffer();
				writeInt(to, strData.filled());
				to->write(strData);
			} else {
				// Just send our ID.
				writeInt(to, 0x05, id);
			}
		}
	}
}
