#pragma once
#include "Thread.h"
#include "Gc/ObjMap.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * An object encapsulating remaining tasks to do after replacing one or more objects.
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
	class ReplaceTasks : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ReplaceTasks();

		// Destroy.
		~ReplaceTasks();

		// Schedule a reference to be replaced.
		void replace(Named *old, Named *with);

	private:
		// References to replace.
		ObjMap<Named> *replaceMap;
	};

}
