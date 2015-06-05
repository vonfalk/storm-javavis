#include "stdafx.h"
#include "DllReader.h"
#include "Package.h"
#include "Engine.h"
#include "BuiltInLoader.h"

namespace storm {
	using namespace dll;

	dll::Reader::Reader(Par<PkgFiles> files, Par<Package> into) : FilesReader(files, into) {}

	FileReader *dll::Reader::createFile(Par<Url> path) {
		return CREATE(DllReader, this, path, pkg);
	}

	DllReader::DllReader(Par<Url> file, Par<Package> into) : FileReader(file, into), loader(null) {
		LibData *data = engine().loadedLibs.load(file);
		if (data)
			loader = data->loader(engine(), into.borrow());
	}

	DllReader::~DllReader() {
		delete loader;
	}

	void DllReader::readTypes() {
		if (loader) {
			loader->loadThreads();
			loader->loadTypes();
		}
	}

	void DllReader::resolveTypes() {
		// Nothing to do...
	}

	void DllReader::readFunctions() {
		if (loader) {
			loader->loadFunctions();
			loader->loadVariables();
		}
	}

}
