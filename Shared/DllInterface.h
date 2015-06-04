#pragma once

namespace storm {

	class Type;
	class Engine;

	/**
	 * Function pointers that makes up the interface between a master Engine and a slave Engine.
	 */
	struct DllInterface {
		typedef Type *(*BuiltIn)(const Engine *, nat id);
		BuiltIn builtIn;

		typedef void (*ObjectFn)(Object *);
		ObjectFn objectCreated, objectDestroyed;

		typedef void *(*AllocObject)(Type *, size_t);
		AllocObject allocObject;

		typedef void (*FreeObject)(void *);
		FreeObject freeObject;

		typedef Engine &(*EngineFrom)(const Object *);
		EngineFrom engineFrom;

		typedef bool (*ObjectIsA)(const Object *, const Type *);
		ObjectIsA objectIsA;

		typedef String (*TypeIdentifier)(const Type *t);
		TypeIdentifier typeIdentifier;

#ifdef DEBUG
		typedef void (*CheckLive)(void *);
		CheckLive checkLive;
#endif
	};

}
