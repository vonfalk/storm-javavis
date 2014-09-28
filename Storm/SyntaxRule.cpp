#include "stdafx.h"
#include "SyntaxRule.h"

namespace storm {

	SyntaxRule::SyntaxRule() {}

	SyntaxRule::~SyntaxRule() {
		clear(tokens);
	}

	void SyntaxRule::output(std::wostream &to) const {
		// output tokens here!
	}

}
