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
			to->write(fullBuffer(d));
		}

		static void writeInt(OStream *to, byte header, Int v) {
			GcPreArray<byte, 5> d;
			d.v[0] = header;
			d.v[1] = byte((Nat(v) >> 24) & 0xFF);
			d.v[2] = byte((Nat(v) >> 16) & 0xFF);
			d.v[3] = byte((Nat(v) >>  8) & 0xFF);
			d.v[4] = byte((Nat(v) >>  0) & 0xFF);
			to->write(fullBuffer(d));
		}

		SExpr::SExpr() {}

		Cons *SExpr::asCons() {
			if (Cons *v = as<Cons>(this))
				return v;
			else
				throw MsgError(L"Not a cons-cell:", this);
		}

		Number *SExpr::asNum() {
			if (Number *v = as<Number>(this))
				return v;
			else
				throw MsgError(L"Not a number:", this);
		}

		String *SExpr::asStr() {
			if (String *v = as<String>(this))
				return v;
			else
				throw MsgError(L"Not a string:", this);
		}

		Symbol *SExpr::asSym() {
			if (Symbol *v = as<Symbol>(this))
				return v;
			else
				throw MsgError(L"Not a symbol:", this);
		}


		void SExpr::write(OStream *to, Connection *c) {
			// Write as 'null'.
			GcPreArray<byte, 1> d;
			d.v[0] = nil;
			to->write(fullBuffer(d));
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

		void Cons::write(OStream *to, Connection *c) {
			// Avoid stack overflow by doing cons-cells iteratively in the case of lists.
			Cons *at = this;
			while (at) {
				GcPreArray<byte, 1> d;
				d.v[0] = cons;
				to->write(fullBuffer(d));

				c->write(to, at->first);

				Cons *next = as<Cons>(at->rest);
				if (!next)
					c->write(to, at->rest);
				at = next;
			}
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

		SExpr *nth(Nat id, SExpr *from) {
			Cons *at = from->asCons();
			for (nat i = 0; i < id; i++)
				at = at->asCons();
			return at->first;
		}

		SExpr *next(SExpr *&pos) {
			Cons *c = pos->asCons();
			pos = c->rest;
			return c->first;
		}

		Number::Number(Int v) : v(v) {}

		void Number::deepCopy(CloneEnv *env) {
			cloned(v, env);
		}

		void Number::toS(StrBuf *to) const {
			*to << v;
		}

		void Number::write(OStream *to, Connection *c) {
			writeInt(to, number, v);
		}

		String::String(const wchar *str) {
			v = new (this) Str(str);
		}

		String::String(Str *v) : v(v) {}

		void String::deepCopy(CloneEnv *env) {
			cloned(v, env);
		}

		void String::toS(StrBuf *to) const {
			*to << L"\"";

			Nat count = 0;
			Str *escaped = v->escape();
			for (Str::Iter i = escaped->begin(), end = escaped->end(); i != end; ++i) {
				*to << i.v();

				if (++count > 50) {
					*to << L"...";
					break;
				}
			}

			*to << L"\"";
			// *to << L"\"" << v->escape() << L"\"";
		}

		void String::write(OStream *to, Connection *c) {
			OMemStream *mem = new (this) OMemStream();
			TextOutput *text = new (this) Utf8Output(mem);
			text->write(v);
			text->flush();

			Buffer strData = mem->buffer();
			writeInt(to, string, strData.filled());
			to->write(strData);
		}


		Symbol::Symbol(Str *v, Nat id) : v(v), id(id) {}

		void Symbol::deepCopy(CloneEnv *env) {
			// No need, we're immutable!
		}

		void Symbol::toS(StrBuf *to) const {
			*to << v->escape(' ');
		}

		Bool Symbol::operator ==(const Symbol &o) const {
			if (!sameType(this, &o))
				return false;

			return id == o.id;
		}

		Nat Symbol::hash() const {
			return natHash(id);
		}

		void Symbol::write(OStream *to, Connection *c) {
			if (c->sendSymbol(this)) {
				// Fist time, send the entire symbol!
				writeInt(to, newSymbol, id);

				OMemStream *mem = new (this) OMemStream();
				TextOutput *text = new (this) Utf8Output(mem);
				text->write(v);
				text->flush();

				Buffer strData = mem->buffer();
				writeInt(to, strData.filled());
				to->write(strData);
			} else {
				// Just send our ID.
				writeInt(to, oldSymbol, id);
			}
		}
	}
}
