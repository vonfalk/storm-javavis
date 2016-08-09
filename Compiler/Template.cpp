#include "stdafx.h"
#include "Template.h"

namespace storm {

	Template::Template(Str *name) : name(name) {}

	Named *Template::generate(SimplePart *par) {
		return null;
	}

}
