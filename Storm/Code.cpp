#include "stdafx.h"
#include "Code.h"

namespace storm {

	void Code::update(code::RefSource &update) {
		if (toUpdate == &update)
			return;
		toUpdate = &update;
		newRef();
	}


	/**
	 * Static code.
	 */

	StaticCode::StaticCode(void *ptr) : ptr(ptr) {}

	void StaticCode::newRef() {
		toUpdate->set(ptr);
	}


	/**
	 * Lazily loaded code.
	 */

	void LazyCode::newRef() {}


	/**
	 * Delegated code.
	 */

	DelegatedCode::DelegatedCode(code::Ref ref, const String &title) : reference(ref, title) {
		reference.onChange = memberFn(this, &DelegatedCode::updated);
	}

	void DelegatedCode::newRef() {
		updated(reference.address());
	}

	void DelegatedCode::updated(void *p) {
		if (toUpdate)
			toUpdate->set(p);
	}

}
