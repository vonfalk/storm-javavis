#pragma once
#include "SyntaxObject.h"
#include "Named.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(core);

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

		// Destroy.
		~NamedThread();

		// Declared at.
		SrcPos pos;

		// Reference to this thread.
		code::Ref ref();

		// Get the thread. (borrowed ptr).
		Thread *thread() const { return myThread.borrow(); }

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// The thread we are representing. Make sure this stays the same over time, otherwise
		// the thread safety is violated. Also double-check this when considering code updates!
		Auto<Thread> myThread;

		// Reference.
		code::RefSource *reference;
	};


}
