#include "stdafx.h"
#include "Template.h"
#include "Type.h"
#include "Core/Str.h"

namespace storm {

	Template::Template(Str *name) : name(name) {}

	Named *Template::generate(SimplePart *part) {
		return null;
	}


	TemplateCppFn::TemplateCppFn(Str *name, GenerateFn fn) : Template(name), fn(fn) {}

	Named *TemplateCppFn::generate(SimplePart *part) {
		ValueArray *va = new (this) ValueArray();
		va->reserve(part->params->count());
		for (nat i = 0; i < part->params->count(); i++) {
			va->push(part->params->at(i));
		}
		return generate(va);
	}

	Type *TemplateCppFn::generate(ValueArray *part) {
		return (*fn)(name, part);
	}


	TemplateFn::TemplateFn(Str *name, Fn<MAYBE(Named *), Str *, SimplePart *> *fn) : Template(name), fn(fn) {}

	Named *TemplateFn::generate(SimplePart *part) {
		return fn->call(name, part);
	}

}
