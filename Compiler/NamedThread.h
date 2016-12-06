#pragma once
#include "Named.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Declares a named thread somewhere. A named thread is simply one specific thread that has a
	 * name in the name tree, and it can therefore be referred to by that name, rather than by an
	 * object.
	 */
	class NamedThread : public Named {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR NamedThread(Str *name);

		// Create, and assign to a specific thread.
		STORM_CTOR NamedThread(Str *name, Thread *thread);

		// Reference to this thread.
		code::Ref ref();

		// Get the thread.
		Thread *STORM_FN thread() const { return myThread; }

		// Output.
		virtual void STORM_FN toS(StrBuf *b) const;

	private:
		// The thread we're referring to.
		Thread *myThread;

		// Reference.
		code::RefSource *reference;
	};

}
