#pragma once
#include "Regex.h"

namespace storm {

	/**
	 * Describes a syntax token. This is an abstract base class. Implementations
	 * can be found below.
	 */
	class SyntaxToken : public Printable, NoCopy {
	public:
		// More public interface here...

	};


	/**
	 * Represents a regex token (might as well be a regular string).
	 */
	class RegexToken : public SyntaxToken {
	public:
		RegexToken(const String &regex, const String &to = L"");

		// 'compiled' regex to match.
		Regex regex;

	protected:
		virtual void output(std::wostream &to) const;

	private:

		// Bind the matched part (or the syntax tree) to this literal. Nothing
		// is done if 'bindTo' is empty.
		String bindTo;
	};

	/**
	 * Represents matching another type, and possibly binding it to a variable.
	 */
	class TypeToken : public SyntaxToken {
	public:
		TypeToken(const String &name, const String &to = L"");

		// Get the type.
		inline const String &type() const { return typeName; }

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Type to match.
		String typeName;

		// Bind the matched part (or the syntax tree) to this literal. Nothing
		// is done if 'bindTo' is empty.
		String bindTo;
	};

	/**
	 * Represents matching the whitespace type.
	 */
	class DelimToken : public TypeToken {
	public:
		DelimToken();
	protected:
		virtual void output(std::wostream &to) const;
	};
}
