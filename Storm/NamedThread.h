#pragma once
#include "SyntaxObject.h"
#include "Named.h"
#include "Thread.h"

namespace storm {

	/**
	 * Declares a named thread in a package.
	 */
	class NamedThread : public Named {
		STORM_CLASS;
	public:
		// Create.
		NamedThread(const String &name);
		NamedThread(const String &name, Par<Thread> t);
		STORM_CTOR NamedThread(Par<Str> name);
		STORM_CTOR NamedThread(Par<SStr> name);

		// Declared at.
		SrcPos pos;

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// The thread we are representing. Make sure this stays the same over time, otherwise
		// the thread safety is violated. Also double-check this when considering code updates!
		Auto<Thread> thread;

	};

}
