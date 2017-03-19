#include "stdafx.h"
#include "Engine.h"
#include "LibData.h"
#include "Core/EngineFwd.h"
#include "Core/SharedLib.h"
#include "Core/Gen/CppTypes.h"

namespace storm {

#define CHECK_SIZE(Type, var)											\
	if (sizeof(Type) != (var)) {										\
		ok = false;														\
		PLN(L"Size of " << STRING(Type) << L" does not match ("			\
			<< sizeof(Type) << L" vs " << (var)	<< L").");				\
		PLN(L"Make sure you are using compatible versions of Storm and libraries."); \
	}

	static void destroyLibInfo(SharedLibInfo *info) {
		destroyLibData(info->libData);
	}

	bool sharedLibEntry(const SharedLibStart &params, SharedLibInfo &out) {
		// Check sizes for version issues.
		bool ok = true;
		CHECK_SIZE(SharedLibStart, params.sizeLibStart);
		CHECK_SIZE(SharedLibInfo, params.sizeLibInfo);
		CHECK_SIZE(EngineFwdShared, params.sizeFwdShared);
		CHECK_SIZE(EngineFwdUnique, params.sizeFwdUnique);
		if (!ok)
			return false;

		void *prev = params.engine.attach(params.shared, params.unique);

		SharedLibInfo result = {
			cppWorld(),
			prev,
			createLibData(params.engine),
			&destroyLibInfo,
		};

		out = result;
		return true;
	}

}
