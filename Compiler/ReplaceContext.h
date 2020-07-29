#pragma once
#include "Thread.h"
#include "Value.h"
#include "Gc/ObjMap.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class NameSet;
	class VTable;

	/**
	 * An object encapsulating various state that is required while reloading code.
	 *
	 * More specifically:
	 * - An equivalence relation between new and old types.
	 */
	class ReplaceContext : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ReplaceContext();

		// Are the two types actually the same?
		Bool STORM_FN same(Type *a, Type *b);

		// Normalize types and/or values.
		Type *STORM_FN normalize(Type *t);
		Value STORM_FN normalize(Value v);

		// Build the type equivalence from two NameSets.
		void STORM_FN buildTypeEquivalence(NameSet *oldRoot, NameSet *newRoot);

	private:
		// Equivalence relation of types (new type -> old type).
		Map<Type *, Type *> *typeEq;

		// Recursive helper for building 'typeEq'.
		void buildTypeEq(NameSet *oldRoot, NameSet *currOld, NameSet *currNew);

		// Resolve parameters during type equivalence.
		Array<Value> *resolveParams(NameSet *root, Array<Value> *params);
		Value resolveParam(NameSet *root, Value param);
	};


	/**
	 * An object encapsulating remaining tasks to do after replacing one or more objects, and other
	 * state that is needed while replacing objects.
	 *
	 * These tasks are things that cannot be made by the replacing objects themselves, for example
	 * since they require the GC to be paused or other similar things. Operations that are
	 * beneficial to perform in batch whenever all replacements have been performed are also queued
	 * up in this object.
	 *
	 * This encompasses:
	 * - Replacing all references of one object with another.
	 * - Modifying the layout of objects in memory.
	 */
	class ReplaceTasks : public ReplaceContext {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ReplaceTasks();

		// Destroy.
		~ReplaceTasks();

		// Schedule a reference to be replaced.
		void STORM_FN replace(Named *old, Named *with);
		void STORM_FN replace(const Handle *old, const Handle *with);
		void replace(const GcType *old, const GcType *with);

		// VTable replacement. This refers to the content of the vtable rather than the vtable itself.
		// VTables are not replaced globally, only the 'vtable' field of the type is actually modified.
		void STORM_FN replace(VTable *old, VTable *with);

		// Apply changes requested. We don't allow doing this from Storm. It is not necessarily
		// good to do while other threads are running.
		void apply();

	private:
		// References to replace.
		RawObjMap *replaceMap;

		// VTables to replace.
		RawObjMap *vtableMap;
	};

}
