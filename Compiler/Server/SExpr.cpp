#include "stdafx.h"
#include "SExpr.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"

namespace storm {
	namespace server {

		SExpr::SExpr() {}

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

	}
}
