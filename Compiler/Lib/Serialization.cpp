#include "stdafx.h"
#include "Serialization.h"
#include "Compiler/Type.h"
#include "Compiler/Engine.h"
#include "Core/Handle.h"
#include "Core/Io/Serialization.h"
#include "Code/Instr.h"
#include "Code/Listing.h"

namespace storm {

	SerializeInfo::SerializeInfo(Function *read, Function *write)
		: read(read), write(write) {}

	MAYBE(SerializeInfo *) serializeInfo(Type *type) {
		Engine &e = type->engine;
		Value me = thisPtr(type);
		Value oStream(StormInfo<ObjOStream>::type(e));
		Value iStream(StormInfo<ObjIStream>::type(e));
		Scope scope = e.scope();

		Function *read = null;
		Function *write = null;
		Function *info = null;


		{
			SimplePart *name = new (e) SimplePart(S("read"));
			name->params->push(iStream);
			if (Function *found = as<Function>(type->findHere(name, scope))) {
				if (Value(type).canStore(found->result))
					read = found;
			}
		}

		{
			SimplePart *name = new (e) SimplePart(S("write"));
			name->params->push(me);
			name->params->push(oStream);
			// We ignore the return type here. It is not important!
			write = as<Function>(type->findHere(name, scope));
		}

		if (read && write)
			return new (type) SerializeInfo(read, write);
		else
			return null;
	}

	Bool serializable(Type *type) {
		return serializeInfo(type) != null;
	}

	Function *serializedTypeFn(SerializedType *type) {
		Engine &e = type->engine();
		Value result(StormInfo<SerializedType>::type(e));

		using namespace code;
		Listing *l = new (e) Listing(false, result.desc(e));
		*l << prolog();
		*l << fnRet(objPtr(type));

		Array<Value> *params = new (e) Array<Value>();
		Function *fn = new (e) Function(result, new (e) Str(S("serializedType")), params);
		fn->make(fnStatic);
		fn->setCode(new (e) DynamicCode(l));
		return fn;
	}

}
