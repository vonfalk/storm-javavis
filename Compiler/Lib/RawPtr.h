#pragma once
#include "Compiler/Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A type that wraps a raw pointer to either a class or an actor type.
	 *
	 * This type allows treating classes and actors as pointers in hash maps, and to extract members
	 * from the pointer without too much type-checking. Intended as a tool for introspection, and
	 * not much more than that.
	 */
	class RawPtrType : public Type {
		STORM_CLASS;
	public:
		RawPtrType(Engine &e);

	protected:
		virtual Bool STORM_FN loadAll();
		virtual code::TypeDesc *STORM_FN createTypeDesc();
		virtual void STORM_FN modifyHandle(Handle *handle);
	};

}
