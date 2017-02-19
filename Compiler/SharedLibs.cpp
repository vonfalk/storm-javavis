#include "stdafx.h"
#include "SharedLibs.h"

namespace storm {

	SharedLibs::SharedLibs() {}

	SharedLibs::~SharedLibs() {
		unload();
	}

	SharedLib *SharedLibs::load(Url *file) {
		SharedLib *loaded = SharedLib::load(file);
		if (!loaded)
			return null;

		if (SharedLib *prev = loaded->prevInstance()) {
			delete loaded;
			return prev;
		}

		this->loaded.push_back(loaded);
		return loaded;
	}

	void SharedLibs::shutdown() {
		for (nat i = 0; i < loaded.size(); i++)
			loaded[i]->shutdown();
	}

	void SharedLibs::unload() {
		for (nat i = 0; i < loaded.size(); i++)
			delete loaded[i];
		loaded.clear();
	}

}
