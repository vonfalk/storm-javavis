#pragma once
#include "Core/Io/Serialization.h"
#include "Compiler/Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Helpers for serialization.
	 */


	// Is the type 't' serializable? Note: It is usually more efficient to call 'serializeInfo' and
	// reuse that information rather than calling 'serializable' first and then 'serializeInfo'.
	Bool STORM_FN serializable(Type *t) ON(Compiler);

	/**
	 * Information about how to serialize a type.
	 */
	class SerializeInfo : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR SerializeInfo(Function *read, Function *write);

		// The 'read' function.
		Function *read;

		// The 'write' function.
		Function *write;
	};

	// Get information about how to serialize a type.
	MAYBE(SerializeInfo *) STORM_FN serializeInfo(Type *t) ON(Compiler);


	// Generate a function that provides the "serializedType" instance provided.
	Function *STORM_FN serializedTypeFn(SerializedType *type);

	// Generate a function that provides the default implementation of the "read" function for
	// serialization. Roughly equivalent to:
	// T read(ObjIStream is) {
	//   return is.readObject(named{T});
	// }
	Function *STORM_FN serializedReadFn(Type *type) ON(Compiler);

}
