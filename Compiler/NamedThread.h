#pragma once
#include "Named.h"
#include "Syntax/SStr.h"

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
		STORM_CTOR NamedThread(SrcPos pos, Str *name);
		STORM_CTOR NamedThread(syntax::SStr *name);

		// Create, and assign to a specific thread.
		STORM_CTOR NamedThread(Str *name, Thread *thread);

		// Reference to this thread.
		code::Ref ref();

		// Get the thread.
		Thread *STORM_FN thread() const { return myThread; }

		// Output.
		virtual void STORM_FN toS(StrBuf *b) const;

		// Replace.
		virtual MAYBE(Str *) STORM_FN canReplace(Named *old, ReplaceContext *ctx);

	protected:
		// Replace.
		virtual void STORM_FN doReplace(Named *old, ReplaceTasks *tasks);

	private:
		// The thread we're referring to.
		Thread *myThread;

		// Reference.
		code::RefSource *reference;
	};

}
