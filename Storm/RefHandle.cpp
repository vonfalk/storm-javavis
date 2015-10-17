#include "stdafx.h"
#include "RefHandle.h"
#include "Shared/Str.h"
#include "OS/FnCall.h"

namespace storm {

	RefHandle::RefHandle(const code::Content &content) :
		destroyUpdater(null),
		createUpdater(null),
		deepCopyUpdater(null),
		equalsUpdater(null),
		hashUpdater(null),
		outputUpdater(null),
		outputMode(outputNone),
		content(content) {}

	RefHandle::~RefHandle() {
		delete destroyUpdater;
		delete createUpdater;
		delete deepCopyUpdater;
		delete equalsUpdater;
		delete hashUpdater;
		delete outputUpdater;
	}

	static void addParam(os::FnParams &to, const Handle &handle, const void *value) {
		to.add(handle.create, handle.destroy, handle.size, handle.isFloat, value);
	}

	void RefHandle::output(const void *obj, StrBuf *to) const {
		switch (outputMode) {
		case outputNone: {
			to->add(L"?");
			break;
		}
		case outputAdd: {
			os::FnParams p;
			p.add(to);
			addParam(p, *this, obj);
			steal(os::call<StrBuf *>(outputFn, true, p));
			break;
		}
		case outputByRefMember: {
			typedef Str *(CODECALL *Fn)(const void *);
			Fn fn = (Fn)outputFn;
			steal(to->add(steal((*fn)(obj))));
			break;
		}
		case outputByRef: {
			// Currently, this is exactly the same as above, but that may change when new
			// architectures are introduced, with new, interesting calling conventions!
			typedef Str *(CODECALL *Fn)(const void *);
			Fn fn = (Fn)outputFn;
			steal(to->add(steal((*fn)(obj))));
			break;
		}
		case outputByVal: {
			os::FnParams p;
			addParam(p, *this, obj);
			steal(to->add(steal(os::call<Str *>(outputFn, false, p))));
			break;
		}
		}
	}

	void RefHandle::destroyRef(const code::Ref &ref) {
		if (destroyUpdater)
			destroyUpdater->set(ref);
		else
			destroyUpdater = new code::AddrReference((void **)&destroy, ref, content);
	}

	void RefHandle::destroyRaw(const void *ptr) {
		del(destroyUpdater);
		destroy = (Destroy)ptr;
	}

	void RefHandle::destroyRef() {
		del(destroyUpdater);
		destroy = null;
	}

	void RefHandle::deepCopyRef(const code::Ref &ref) {
		if (deepCopyUpdater)
			deepCopyUpdater->set(ref);
		else
			deepCopyUpdater = new code::AddrReference((void **)&deepCopy, ref, content);
	}

	void RefHandle::deepCopyRaw(const void *ptr) {
		del(deepCopyUpdater);
		deepCopy = (DeepCopy)ptr;
	}

	void RefHandle::deepCopyRef() {
		del(deepCopyUpdater);
		deepCopy = null;
	}

	void RefHandle::createRef(const code::Ref &ref) {
		if (createUpdater)
			createUpdater->set(ref);
		else
			createUpdater = new code::AddrReference((void **)&create, ref, content);
	}

	void RefHandle::createRaw(const void *ptr) {
		del(createUpdater);
		create = (Create)ptr;
	}

	void RefHandle::equalsRef() {
		del(equalsUpdater);
		equals = null;
	}

	void RefHandle::equalsRef(const code::Ref &ref) {
		if (equalsUpdater)
			equalsUpdater->set(ref);
		else
			equalsUpdater = new code::AddrReference((void **)&equals, ref, content);
	}

	void RefHandle::hashRef() {
		del(hashUpdater);
		hash = null;
	}

	void RefHandle::hashRef(const code::Ref &ref) {
		if (hashUpdater)
			hashUpdater->set(ref);
		else
			hashUpdater = new code::AddrReference((void **)&hash, ref, content);
	}

	void RefHandle::outputRef() {
		del(outputUpdater);
		outputFn = null;
		outputMode = outputNone;
	}

	void RefHandle::outputRef(const code::Ref &ref, OutputMode mode) {
		if (outputUpdater)
			outputUpdater->set(ref);
		else
			outputUpdater = new code::AddrReference((void **)&outputFn, ref, content);
		outputMode = mode;
	}

}
