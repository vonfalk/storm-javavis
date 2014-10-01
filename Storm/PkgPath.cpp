#include "stdafx.h"
#include "PkgPath.h"

namespace storm {

	PkgPath::PkgPath() {}

	PkgPath::PkgPath(const String &path) {
		parts = path.toLower().split(L".");
	}

	PkgPath &PkgPath::operator +=(const PkgPath &o) {
		for (nat i = 0; i < o.parts.size(); i++)
			parts.push_back(o.parts[i]);
		return *this;
	}

	PkgPath PkgPath::operator +(const PkgPath &o) const {
		PkgPath t(*this);
		t += o;
		return t;
	}

	PkgPath PkgPath::parent() const {
		PkgPath p(*this);
		if (!root())
			p.parts.pop_back();
		return p;
	}

	void PkgPath::output(std::wostream &to) const {
		to << join(parts, L".");
	}
}
