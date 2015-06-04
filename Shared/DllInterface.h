#pragma once

namespace storm {

	class Type;
	class Engine;

	/**
	 * Function pointers that makes up the interface between a master Engine and a slave Engine.
	 */
	struct DllInterface {
		typedef Type *(*BuiltIn)(Engine &e, void *data, nat id);
		BuiltIn builtIn;

		// Data to the lookup-function. Specific to this DLL.
		void *data;

		typedef void (*ObjectFn)(Object *o);
		ObjectFn objectCreated, objectDestroyed;

		typedef void *(*AllocObject)(Type *type, size_t cppSize);
		AllocObject allocObject;

		typedef void (*FreeObject)(void *mem);
		FreeObject freeObject;

		typedef Engine &(*EngineFrom)(const Object *o);
		EngineFrom engineFrom;

		typedef bool (*ObjectIsA)(const Object *o, const Type *t);
		ObjectIsA objectIsA;

		typedef String (*TypeIdentifier)(const Type *t);
		TypeIdentifier typeIdentifier;

#ifdef DEBUG
		typedef void (*CheckLive)(void *mem);
		CheckLive checkLive;
#endif
	};

}
