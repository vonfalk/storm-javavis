#include "stdafx.h"
#include "ArrayTemplate.h"

namespace storm {

	static Named *generateArray(Par<NamePart> part) {
		PLN("Generating array: " << part);
		return null;
	}

	Template *arrayTemplate(Engine &e) {
		return CREATE(Template, e, L"Array", simpleFn(&generateArray));
	}

}
