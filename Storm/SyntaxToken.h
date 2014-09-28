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
		RegexToken(const String &regex);

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// 'compiled' regex to match.
		Regex regex;
	};

	/**
	 * Represents matching another type, and possibly binding it to a variable.
	 */
	class TypeToken : public SyntaxToken {
	public:
		TypeToken(const String &name, const String &to = L"");

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Type to match.
		String typeName;

		// Bind to? (empty -> nothing).
		String bindTo;
	};
}
