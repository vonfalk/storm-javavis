#include "stdafx.h"
#include "DllEngine.h"
#ifdef STORM_DLL

namespace storm {

	static const DllInterface *interface = null;

	void Engine::setup(const DllInterface *i) {
		interface = i;
	}

	Type *Engine::builtIn(nat id) {
		return (*interface->builtIn)(this, id);
	}

}

#endif
