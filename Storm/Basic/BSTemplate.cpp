#include "stdafx.h"
#include "BSTemplate.h"
#include "Exception.h"
#include "Shared/Str.h"

namespace storm {

	bs::TemplateAdapter::TemplateAdapter(Auto<Str> name) : Template(name) {
		data = CREATE(ArrayP<Template>, this);
	}

	bs::TemplateAdapter::TemplateAdapter(const String &name) : Template(name) {
		data = CREATE(ArrayP<Template>, this);
	}

	void bs::TemplateAdapter::output(wostream &to) const {
		for (nat i = 0; i < data->count(); i++) {
			to << data->at(i);
		}
	}

	Named *bs::TemplateAdapter::generate(Par<NamePart> par) {
		Auto<Named> result;
		for (nat i = 0; i < data->count(); i++) {
			Auto<Named> now = data->at(i)->generate(par);
			if (now) {
				if (result)
					throw TypedefError(L"The template " + ::toS(par) + L" is ambiguous.");

				result = now;
			}
		}

		return result.ret();
	}

	void bs::TemplateAdapter::add(Par<Template> t) {
		data->push(t);
	}

}
