#pragma once
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Core/Array.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Messages sent to and from the client. This is based on S-Expressions from LISP.
		 */
		class SExpr : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR SExpr();
		};


		/**
		 * Cons-cell.
		 */
		class Cons : public SExpr {
			STORM_CLASS;
		public:
			STORM_CTOR Cons();
			STORM_CTOR Cons(MAYBE(SExpr *) first, MAYBE(SExpr *) rest);

			MAYBE(SExpr *) first;
			MAYBE(SExpr *) rest;

			virtual void STORM_FN deepCopy(CloneEnv *env);
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		// Convenience functions.
		Cons *STORM_FN cons(EnginePtr e, MAYBE(SExpr *) first, MAYBE(SExpr *) rest);
		MAYBE(Cons *) STORM_FN list(Array<SExpr *> *data);
		MAYBE(Cons *) list(Engine &e, Nat count, ...);

		/**
		 * Number.
		 */
		class Number : public SExpr {
			STORM_CLASS;
		public:
			STORM_CTOR Number(Int v);

			Int v;

			virtual void STORM_FN deepCopy(CloneEnv *env);
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * String.
		 */
		class String : public SExpr {
			STORM_CLASS;
		public:
			String(const wchar *str);
			STORM_CTOR String(Str *v);

			Str *v;

			virtual void STORM_FN deepCopy(CloneEnv *env);
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
