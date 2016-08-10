#include "stdafx.h"
#include "Template.h"
#include "Type.h"

namespace storm {

	Template::Template(Str *name) : name(name) {}

	Named *Template::generate(SimplePart *part) {
		return null;
	}


	TemplateFn::TemplateFn(Str *name, GenerateFn fn) : Template(name), fn(fn) {}

	Named *TemplateFn::generate(SimplePart *part) {
		ValueArray *va = new (this) ValueArray();
		va->reserve(part->params->count());
		for (nat i = 0; i < part->params->count(); i++) {
			va->push(part->params->at(i));
		}
		return generate(va);
	}

	Type *TemplateFn::generate(ValueArray *part) {
		return (*fn)(name, part);
	}

}
