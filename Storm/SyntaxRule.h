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

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// List of all tokens to match.
		vector<SyntaxToken*> tokens;
	};

}
