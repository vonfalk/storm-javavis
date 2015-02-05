#include "stdafx.h"
#include "ArrayTemplate.h"

namespace storm {

	static Named *generateArray(Par<NamePart> part) {
		if (part->params.size() != 1)
			return null;

		const Value &type = part->params[0];
		PLN("Want to generate for " << type);

		return null;
	}

	Template *arrayTemplate(Engine &e) {
		return CREATE(Template, e, L"Array", simpleFn(&generateArray));
	}

}
