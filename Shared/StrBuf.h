#pragma once
#include "Object.h"
#include "Utils/Templates.h"

namespace storm {
	STORM_PKG(core);

	class Str;

	/**
	 * Mutable string buffer for building strings easily and efficiently.
	 */
	class StrBuf : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR StrBuf();
		STORM_CTOR StrBuf(Par<StrBuf> o);
		STORM_CTOR StrBuf(Par<Str> str);

		// Copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Destroy.
		~StrBuf();

		// Get the string we've built!
		virtual Str *STORM_FN toS();

		// Append stuff.
		virtual StrBuf *STORM_FN add(Par<Str> str);
		virtual StrBuf *STORM_FN add(Int i);
		virtual StrBuf *STORM_FN add(Nat i);
		virtual StrBuf *STORM_FN add(Long i);
		virtual StrBuf *STORM_FN add(Word i);
		virtual StrBuf *STORM_FN add(Byte i);
		virtual StrBuf *STORM_FN add(Float i);

		// Help the templating below...
		StrBuf *CODECALL add(Str *str);

		// C++-specific.
		void add(const String &v);
		void add(const wchar *v);

		// Add a single char.
		virtual void STORM_FN addChar(Nat c);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Buffer. This will always have room for a null-terminator, but may not always be set.
		wchar *buffer;

		// Current capacity of 'buffer'. Not including the null-terminator.
		nat capacity;

		// Current position in 'buffer'.
		nat pos;

		// Ensure capacity.
		void ensure(nat capacity);

		// Insert a null terminator.
		void nullTerminate() const;
	};


	/**
	 * Template magic detecting if type X can be outputted using 'add' on StrBuf directly.
	 */
	template <class T>
	class CanOutput {
	private:
		template <class U, U> struct Check;

		template <class U>
		static YesType check(Check<StrBuf *(CODECALL StrBuf::*)(U), &StrBuf::add> *);

		// template <class U>
		// static YesType check(Check<StrBuf *(CODECALL StrBuf::*)(Par<U>), &StrBuf::add> *);

		template <class U>
		static NoType check(...);

	public:
		enum { v = sizeof(check<typename AsPar<T>::v>(0)) == sizeof(YesType) };
	};

}
