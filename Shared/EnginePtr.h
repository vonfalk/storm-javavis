#pragma once

namespace storm {

	class Engine;

	/**
	 * Object used as the first parameter to STORM_ENGINE_FN. The wrapping utility needs
	 * to store some extra data on the stack, which this class accommodates.
	 */
	class EnginePtr {
	public:
		// Ctor, so that we can call EnginePtr functions from C++ as well.
		inline EnginePtr(Engine &e) : v(e) {}

		// The engine itself.
		Engine &v;

	private:
		// Additional data required:

#ifdef X86
		// The return address and the return value, in some order.
		void *returnData[2];
#endif
	};

}
