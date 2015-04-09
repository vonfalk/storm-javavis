#pragma once
#include "Object.h"
#include "Str.h"

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

		// Add a single char.
		virtual void STORM_FN addChar(Int c);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Buffer. This will always be null-terminated!
		wchar *buffer;

		// Current capacity of 'buffer'. Not including the null-terminator.
		nat capacity;

		// Current position in 'buffer'.
		nat pos;

		// Ensure capacity.
		void ensure(nat capacity);
	};

}
