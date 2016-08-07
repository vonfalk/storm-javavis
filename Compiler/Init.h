#pragma once
#include "RootArray.h"
#include "TemplateList.h"

namespace storm {

	/**
	 * Initialize the type system.
	 */
	void initTypes(Engine &e, RootArray<Type> &types, RootArray<TemplateList> &templates);

}
