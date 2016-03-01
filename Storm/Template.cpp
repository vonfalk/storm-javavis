#include "stdafx.h"
#include "Template.h"
#include "Shared/Str.h"

namespace storm {

	Template::Template(Par<Str> name) : name(name->v) {}

	Template::Template(const String &name) : name(name) {}

	Template::Template(const String &name, Fn<Named *, Par<FoundParams>> fn) : name(name), generateFn(fn) {}

	Named *Template::generate(Par<FoundParams> part) {
		return generateFn(part);
	}

}
