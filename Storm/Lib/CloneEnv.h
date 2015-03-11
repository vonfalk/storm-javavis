#pragma once
#include "Object.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Keeps track of objects already cloned.
	 */
	class CloneEnv : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR CloneEnv();

		// Destroy
		~CloneEnv();

		// If 'o' was cloned before, get the clone of it. Otherwise returns null.
		Object *STORM_FN cloned(Par<Object> o);

		// Tell us that 'o' is cloned into 'to'
		void STORM_FN cloned(Par<Object> o, Par<Object> to);

	private:
		typedef hash_map<Object *, Object *> ObjMap;
		ObjMap objs;
	};

}
