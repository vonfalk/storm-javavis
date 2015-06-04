#include "stdafx.h"
#include "DllReader.h"
#include "Package.h"
#include "Engine.h"

namespace storm {
	using namespace dll;

	dll::Reader::Reader(Par<PkgFiles> files, Par<Package> into) : FilesReader(files, into) {}

	FileReader *dll::Reader::createFile(Par<Url> path) {
		return CREATE(DllReader, this, path, pkg);
	}

	DllReader::DllReader(Par<Url> file, Par<Package> into) : FileReader(file, into) {
		contents = engine().dynamicLibs.load(file);
	}

	void DllReader::readTypes() {}

	void DllReader::resolveTypes() {}

	void DllReader::readFunctions() {}

}
