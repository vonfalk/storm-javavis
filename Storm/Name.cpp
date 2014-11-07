#include "stdafx.h"
#include "Name.h"

namespace storm {

	Name::Name() {}

	Name::Name(const String &path) {
		parts = path.split(L".");
	}

	Name &Name::operator +=(const Name &o) {
		for (nat i = 0; i < o.parts.size(); i++)
			parts.push_back(o.parts[i]);
		return *this;
	}

	Name Name::operator +(const Name &o) const {
		Name t(*this);
		t += o;
		return t;
	}

	Name Name::parent() const {
		Name p(*this);
		if (!root())
			p.parts.pop_back();
		return p;
	}

	String Name::last() const {
		return parts.back();
	}

	void Name::output(std::wostream &to) const {
		join(to, parts, L".");
	}
}
