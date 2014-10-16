#pragma once
#include "Regex.h"

namespace storm {

	/**
	 * Describes a syntax token. This is an abstract base class. Implementations
	 * can be found below.
	 */
	class SyntaxToken : public Printable, NoCopy {
	public:
		SyntaxToken(const String &to, bool method);

		// Bind the matched part (or the syntax tree) to this literal. Nothing
		// is done if 'bindTo' is empty.
		String bindTo;

		// Instead of variable, run method?
		bool method;

		// Bind to something?
		inline bool bind() const { return bindTo != L""; }

		// More public interface here...

	protected:
		virtual void output(std::wostream &to) const;
	};


	/**
	 * Represents a regex token (might as well be a regular string).
	 */
	class RegexToken : public SyntaxToken {
	public:
		RegexToken(const String &regex, const String &to = L"", bool method = false);

		// 'compiled' regex to match.
		Regex regex;

	protected:
		virtual void output(std::wostream &to) const;
	};

	/**
	 * Represents matching another type, and possibly binding it to a variable.
	 */
	class TypeToken : public SyntaxToken {
	public:
		TypeToken(const String &name, const String &to = L"", bool method = false);

		// Get the type.
		inline const String &type() const { return typeName; }

		// Parameters to the type (if any).
		vector<String> params;

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Type to match.
		String typeName;
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
