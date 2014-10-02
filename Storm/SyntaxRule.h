#pragma once
#include "SyntaxToken.h"

namespace storm {

	/**
	 * Describes a syntax rule.
	 */
	class SyntaxRule : public Printable, NoCopy {
	public:
		SyntaxRule();
		~SyntaxRule();

		// What kind of repeat is to be used?
		enum Repeat {
			rNone,
			rZeroOne,
			rZeroPlus,
			rOnePlus,
		};

		// Add a token.
		void add(SyntaxToken *token);

		// Set start/end of repeat (only one is supported).
		void startRepeat();
		void endRepeat(Repeat type);

		// Has any repeat?
		inline bool hasRepeat() const { return repType != rNone; }

		// Set function to call on match.
		void setMatchFn(const String &name, const vector<String> &params);

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// List of all tokens to match.
		vector<SyntaxToken*> tokens;

		// Start/end of repeat.
		nat repStart, repEnd;
		Repeat repType;

		// Function name to call.
		String matchFn;
		vector<String> matchFnParams;

		// Helper for repetition outputs.
		void outputRep(std::wostream &to, nat i) const;
	};

}
