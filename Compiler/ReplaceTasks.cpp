#include "stdafx.h"
#include "ReplaceTasks.h"
#include "Engine.h"

namespace storm {

	ReplaceTasks::ReplaceTasks() {
		replaceMap = new ObjMap<Named>(engine().gc);
	}

	ReplaceTasks::~ReplaceTasks() {
		delete replaceMap;
	}

	void ReplaceTasks::replace(Named *old, Named *with) {
		replaceMap->put(old, with);
	}

}
