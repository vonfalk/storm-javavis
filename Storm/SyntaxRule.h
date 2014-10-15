#pragma once
#include "SyntaxOption.h"

namespace storm {

	/**
	 * Describes a syntax type. A type consists of zero or more syntax rules. A syntax
	 * type may then be matched to one of the rules present.
	 */
	class SyntaxRule : public Printable, NoCopy {
	public:
		// Create a new syntax type.
		SyntaxRule(const String &name);

		~SyntaxRule();

		// Set how to output this type (assumes unescaped string, ie containing \n and so on).
		void setOutput(const String &str);

		// Add a rule. Takes ownership of the pointer.
		void add(SyntaxOption *rule);

		// Options access.
		inline nat size() const { return options.size(); }
		inline SyntaxOption* operator[] (nat i) { return options[i]; }

		// Get our name.
		inline const String &name() const { return tName; }
	protected:
		virtual void output(std::wostream &to) const;

	private:
		// The name of this type.
		String tName;

		// List of all options associated to this type.
		vector<SyntaxOption*> options;

		// How to output matches. null=>default.
		String *outputStr;
	};

}
