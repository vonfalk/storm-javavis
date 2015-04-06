#include "stdafx.h"
#include "AsmWrap.h"

namespace storm {

	wrap::Instruction::Instruction(const code::Instruction &v) : v(v) {}

	wrap::Instruction wrap::prolog() {
		return Instruction(code::prolog());
	}

	wrap::Instruction wrap::epilog() {
		return Instruction(code::epilog());
	}

	wrap::Instruction wrap::ret() {
		TODO(L"Require a size here!");
		return Instruction(code::ret(Size::sPtr));
	}

	wrap::Listing::Listing() {}

	wrap::Listing::Listing(Par<Listing> o) : v(o->v) {}

	wrap::Listing *wrap::Listing::operator <<(const Instruction &v) {
		TODO(L"Do not automatically set ptrA to null here.");
		this->v << v.v;
		this->v << code::mov(code::ptrA, code::intPtrConst(0));
		addRef();
		return this;
	}

}
