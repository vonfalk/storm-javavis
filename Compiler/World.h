#pragma once
#include "RootArray.h"
#include "TemplateList.h"
#include "Type.h"
#include "Core/Thread.h"

namespace storm {

	/**
	 * Holds data about a set of types accessible from C++. Each of the types and templates have
	 * their own unique id:s.
	 */
	class World : NoCopy {
	public:
		World(Gc &gc);

		// All non-template types.
		RootArray<Type> types;

		// All template types.
		RootArray<TemplateList> templates;

		// All threads.
		RootArray<Thread> threads;

		// Instantiated 'NamedThread' objects.
		RootArray<NamedThread> namedThreads;

		// Clear all allocations.
		void clear();

		// For each named object:
		typedef void (*NamedFn)(Named *);
		void forNamed(NamedFn fn);
	};

}
