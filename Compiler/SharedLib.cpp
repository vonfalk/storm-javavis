#include "stdafx.h"
#include "SharedLib.h"
#include "Core/Str.h"
#include "Engine.h"
#include "Exception.h"

namespace storm {

	SharedLib::SharedLib(Url *file, LoadedLib lib, EntryFn entry) : lib(lib), info(null), world(file->engine().gc) {
		EngineFwdUnique unique = {
			&SharedLib::cppType,
			&SharedLib::cppTemplate,
			&SharedLib::getThread,
			this,
			1.0f,
		};
		SharedLibStart start = {
			file->engine(),
			engineFwd(),
			unique,
		};

		info = (*entry)(&start);
		if (!info)
			throw InternalError(L"Failed to initialize the shared library: " + ::toS(file));
	}

	SharedLib::~SharedLib() {
		if (info && info->destroyFn)
			(*info->destroyFn)(info);

		if (lib != invalidLib)
			unloadLibrary(lib);
	}

	void SharedLib::shutdown() {
		if (info->shutdownFn)
			(*info->shutdownFn)(info);
	}

	SharedLib *SharedLib::prevInstance() {
		if (!info)
			return null;

		return (SharedLib *)info->previousIdentifier;
	}

	CppLoader SharedLib::createLoader(Engine &e, Package *into) {
		return CppLoader(e, info->world, world, into);
	}

	SharedLib *SharedLib::load(Url *file) {
		const char *entryName = SHORT_STRING(SHARED_LIB_ENTRY);

		// Do not load the library unless we actually think we might be able to load something from there!
		if (!hasExport(file, entryName))
			return null;

		LoadedLib lib = loadLibrary(file->format()->c_str());
		if (lib == invalidLib)
			return null;

		EntryFn entry = (EntryFn)findLibraryFn(lib, entryName);
		if (!entry) {
			unloadLibrary(lib);
			return null;
		}

		return new SharedLib(file, lib, entry);
	}


	Type *SharedLib::cppType(Engine &e, void *lib, Nat id) {
		SharedLib *me = (SharedLib *)lib;
		assert(false, L"Implement me!");
		return null;
	}

	Type *SharedLib::cppTemplate(Engine &e, void *lib, Nat id, Nat count, va_list params) {
		SharedLib *me = (SharedLib *)lib;
		assert(false, L"Implement me! Note: the TemplateList objects need to operate on our world's list of types!");
		return null;
	}

	Thread *SharedLib::getThread(Engine &e, void *lib, const DeclThread *decl) {
		SharedLib *me = (SharedLib *)lib;
		assert(false, L"Implement me!");
		return null;
	}

}
