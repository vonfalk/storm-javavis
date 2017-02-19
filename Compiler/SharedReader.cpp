#include "stdafx.h"
#include "SharedReader.h"
#include "Engine.h"

namespace storm {
	namespace shared {

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			return new (files) SharedReader(files, pkg);
		}

		SharedReader::SharedReader(Array<Url *> *files, Package *pkg) : PkgReader(files, pkg) {
			toLoad = null;
		}

		void SharedReader::readTypes() {
			load();
			for (Nat i = 0; i < toLoad->count(); i++) {
				toLoad->at(i).loadTypes();
				toLoad->at(i).loadThreads();
				toLoad->at(i).loadTemplates();
				toLoad->at(i).loadPackages();
			}
		}

		void SharedReader::resolveTypes() {
			load();
			for (Nat i = 0; i < toLoad->count(); i++) {
				toLoad->at(i).loadSuper();
				toLoad->at(i).loadVariables();
			}
		}

		void SharedReader::readFunctions() {
			load();
			for (Nat i = 0; i < toLoad->count(); i++) {
				toLoad->at(i).loadFunctions();
			}
		}

		void SharedReader::load() {
			if (toLoad)
				return;

			toLoad = new (this) Array<CppLoader>();
			for (Nat i = 0; i < files->count(); i++) {
				SharedLib *lib = engine().loadShared(files->at(i));
				toLoad->push(lib->createLoader(engine()));
			}
		}

	}
}
