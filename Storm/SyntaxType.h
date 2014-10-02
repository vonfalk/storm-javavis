#pragma once
#include "SyntaxRule.h"

namespace storm {

	/**
	 * Describes a syntax type. A type consists of zero or more syntax rules. A syntax
	 * type may then be matched to one of the rules present.
	 */
	class SyntaxType : public Printable, NoCopy {
	public:
		// Create a new syntax type.
		SyntaxType(const String &name);

		~SyntaxType();

		// Set how to output this type (assumes unescaped string, ie containing \n and so on).
		void setOutput(const String &str);

		// Add a rule. Takes ownership of the pointer.
		void add(SyntaxRule *rule);

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// The name of this type.
		String name;

		// List of all rules associated to this type.
		vector<SyntaxRule*> rules;

		// How to output matches. null=>default.
		String *outputStr;
	};

}
