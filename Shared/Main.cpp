#include "stdafx.h"
#include "Engine.h"
#include "LibData.h"
#include "Core/EngineFwd.h"
#include "Core/SharedLib.h"
#include "Core/Gen/CppTypes.h"

namespace storm {

	static void destroyLibInfo(SharedLibInfo *info) {
		destroyLibData(info->libData);
	}

	SharedLibInfo sharedLibEntry(const SharedLibStart *params) {
		void *prev = params->engine.attach(params->shared, params->unique);

		SharedLibInfo result = {
			cppWorld(),
			prev,
			createLibData(params->engine),
			&destroyLibInfo,
		};

		return result;
	}

}
