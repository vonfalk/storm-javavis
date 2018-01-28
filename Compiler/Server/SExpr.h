#pragma once
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Core/Array.h"
#include "Core/Io/Stream.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		class Connection;
		class Cons;
		class Number;
		class String;
		class Symbol;

		/**
		 * Messages sent to and from the client. This is based on S-Expressions from LISP.
		 */
		class SExpr : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR SExpr();

			/**
			 * Tags in the stream for the different objects.
			 */
			enum Tag {
				nil = 0x00,
				cons = 0x01,
				number = 0x02,
				string = 0x03,
				newSymbol = 0x04,
				oldSymbol = 0x05,
			};

			// Expect this SExpr to be of some specific sub-type. Throws an exception if not.
			Cons *asCons();
			Number *asNum();
			String *asStr();
			Symbol *asSym();

		protected:
			// Write to a stream.
			virtual void STORM_FN write(OStream *to, Connection *sym);

			friend class Connection;
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
		protected:
			// Write to a stream.
			virtual void STORM_FN write(OStream *to, Connection *sym);
		};

		// Convenience functions.
		Cons *STORM_FN cons(EnginePtr e, MAYBE(SExpr *) first, MAYBE(SExpr *) rest);
		MAYBE(Cons *) STORM_FN list(Array<SExpr *> *data);
		MAYBE(Cons *) list(Engine &e, Nat count, ...);

		// Access elements.
		SExpr *STORM_FN nth(Nat id, SExpr *l);

		// Extract elements of a list one at a time.
		SExpr *next(SExpr *&pos);

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
		protected:
			// Write to a stream.
			virtual void STORM_FN write(OStream *to, Connection *sym);
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
		protected:
			// Write to a stream.
			virtual void STORM_FN write(OStream *to, Connection *sym);
		};

		/**
		 * Symbol.
		 *
		 * Very similar to a string in this representation, but the underlying Symbol objects are
		 * shared. If we ever create a symbol type for Storm, we should use it here!
		 */
		class Symbol : public SExpr {
			STORM_CLASS;
		public:
			virtual void STORM_FN deepCopy(CloneEnv *env);
			virtual void STORM_FN toS(StrBuf *to) const;
			virtual Bool STORM_FN operator ==(const Symbol &o) const;
			virtual Nat STORM_FN hash() const;

		protected:
			// Write to a stream.
			virtual void STORM_FN write(OStream *to, Connection *sym);

		private:
			Symbol(Str *v, Nat id);

			// String.
			Str *v;

			// The symbol ID this symbol is assigned.
			Nat id;

			friend class Connection;
		};

		/**
		 * Exception.
		 */
		class MsgError : public Exception {
		public:
			MsgError(const ::String &what, SExpr *expr) : msg(what + L" " + ::toS(expr)) {}
			::String what() const { return msg; }
		private:
			::String msg;
		};

	}
}
