#include "stdafx.h"
#include "FnPtr.h"
#include "Shared/TObject.h"
#include "OS/Future.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	class RefTarget : public FnPtrBase::Target {
	public:
		RefTarget(const code::Ref &ref) : ref(ref) {}

		code::Ref ref;

		virtual void cloneTo(void *to, size_t size) const {
			assert(size >= sizeof(RefTarget));
			new (to) RefTarget(*this);
		}

		virtual const void *target() const {
			return ref.address();
		}

		virtual void output(wostream &to) const {
			to << ref.targetName();
		}
	};

	FnPtrBase *createRawFnPtr(Type *type, void *refData,
							Thread *t, Object *thisPtr,
							bool strongThis, bool member) {
		Engine &e = type->engine;
		RefTarget target(code::Ref::fromLea(e.arena, refData));
		FnPtrBase *result = new (type) FnPtrBase(target, t, member, thisPtr, strongThis);
		setVTable(result);
		return result;
	}


	FnPtrBase *createFnPtr(Engine &e, const vector<Value> &params,
						const code::Ref &to, Thread *t, Object *thisPtr,
						bool strongThis, bool member) {
		Type *type = fnPtrType(e, params);
		RefTarget target(to);
		FnPtrBase *result = new (type) FnPtrBase(target, t, member, thisPtr, strongThis);
		setVTable(result);
		return result;
	}

}
