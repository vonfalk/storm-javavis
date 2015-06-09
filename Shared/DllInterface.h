#pragma once
#include "Value.h"
#include "OS/Shared.h"

namespace storm {

	class Type;
	class Engine;

	/**
	 * Function pointers that makes up the interface between a master Engine and a slave Engine.
	 */
	struct DllInterface {
		os::OsFns osFns;

		typedef Type *(*BuiltIn)(Engine &e, void *data, nat id);
		BuiltIn builtIn;

		typedef void *(*CppVTable)(void *data, nat id);
		CppVTable cppVTable;

		// Get the library-defined data.
		typedef void *(*GetData)(Engine &e, void *data);
		GetData getData;

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

		typedef vector<ValueData> (*TypeParams)(const Type *t);
		TypeParams typeParams;

		typedef void (*SetVTable)(Object *to);
		SetVTable setVTable;

		typedef bool (*IsClass)(Type *t);
		IsClass isClass;

		typedef Object *(CODECALL *CloneObjectEnv)(Object *o, CloneEnv *env);
		CloneObjectEnv cloneObjectEnv;

		typedef Type *(*StdType)(Engine &e);
		StdType intType, natType, byteType, boolType;

		typedef Type *(*ArrayType)(Engine &e, const ValueData &v);
		ArrayType arrayType;

		typedef Type *(*FutureType)(Engine &e, const ValueData &v);
		FutureType futureType;

		typedef Type *(*FnPtrType)(Engine &e, const vector<ValueData> &params);
		FnPtrType fnPtrType;

		typedef bool (*ToSOverridden)(const Object *o);
		ToSOverridden toSOverridden;

#ifdef DEBUG
		typedef void (*CheckLive)(void *mem);
		CheckLive checkLive;
#endif
	};

	struct BuiltIn;

	/**
	 * Data returned from the DLL entry-point.
	 */
	struct DllInfo {
		// Built in functions, variables and so on.
		const BuiltIn *builtIn;

		// Data object created for this instance.
		void *data;

		// Function to destroy the data at shutdown.
		typedef void (*DestroyFn)(void *);
		DestroyFn destroyData;
	};

}
