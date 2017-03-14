#include "stdafx.h"
#include "EngineFwd.h"
#include "Runtime.h"
#include "OS/Shared.h"

namespace storm {

	const EngineFwdShared &engineFwd() {
		static const EngineFwdShared fwd = {
			&runtime::typeHandle,
			&runtime::voidHandle,
			&runtime::typeOf,
			&runtime::gcTypeOf,
			&runtime::typeName,
			&runtime::isA,
			&runtime::allocEngine,
			&runtime::allocRaw,
			&runtime::allocStaticRaw,
			&runtime::allocObject,
			&runtime::allocArray,
			&runtime::allocWeakArray,
			&runtime::allocCode,
			&runtime::codeSize,
			&runtime::codeRefs,
			&runtime::setVTable,
			&runtime::threadGroup,
			&runtime::createWatch,
			&runtime::postStdRequest,
			&runtime::cloneObject,
			&runtime::cloneObjectEnv,

			// Others.
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
