#include "stdafx.h"
#include "EngineFwd.h"
#include "Runtime.h"
#include "OS/Shared.h"

namespace storm {

	static Thread *compilerGetThread(Engine &e, const DeclThread *decl) {
		return decl->thread(e);
	}

	const EngineFwd &engineFwd() {
		static const EngineFwd fwd = {
			&runtime::cppType,
			&runtime::cppTemplateVa,
			&runtime::typeHandle,
			&runtime::voidHandle,
			&runtime::typeOf,
			&runtime::gcTypeOf,
			&runtime::typeName,
			&runtime::isA,
			&runtime::allocEngine,
			&runtime::allocObject,
			&runtime::allocArray,
			&runtime::allocWeakArray,
			&runtime::allocCode,
			&runtime::codeSize,
			&runtime::codeRefs,
			&runtime::setVTable,
			&runtime::threadGroup,
			&runtime::createWatch,
			&runtime::attachThread,
			&runtime::detachThread,
			&runtime::reattachThread,
			&runtime::postStdRequest,
			&runtime::cloneObject,
			&runtime::cloneObjectEnv,

			// Others.
			&compilerGetThread,
			&os::currentThreadData,
			&os::currentThreadData,
			&os::currentUThreadState,
			&os::currentUThreadState,
			&os::threadCreated,
			&os::threadTerminated,

			// Dummy.
			1.0f,
		};

		return fwd;
	}
}
