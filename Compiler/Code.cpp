#include "stdafx.h"
#include "Code.h"
#include "Engine.h"

namespace storm {

	Code::Code() : toUpdate(null), owner(null) {}

	void Code::attach(Function *to) {
		owner = to;
	}

	void Code::detach() {
		owner = null;
	}

	void Code::update(code::RefSource *update) {
		if (toUpdate == update)
			return;
		toUpdate = update;
		newRef();
	}

	void Code::compile() {}

	void Code::newRef() {}


	/**
	 * Static code.
	 */

	StaticCode::StaticCode(const void *ptr) : ptr(ptr) {}

	void StaticCode::newRef() {
		toUpdate->setPtr(ptr);
	}


	/**
	 * Dynamic code.
	 */

	DynamicCode::DynamicCode(code::Listing *code) {
		binary = new (this) code::Binary(engine().arena(), code);
	}

	void DynamicCode::newRef() {
		toUpdate->set(binary);
	}


	/**
	 * Delegated code.
	 */

	DelegatedCode::DelegatedCode(code::Ref ref) {
		content = new (this) code::Content();
		this->ref = new (this) DelegatedRef(this, ref, content);
	}

	void DelegatedCode::newRef() {
		toUpdate->set(content);
	}

	void DelegatedCode::moved(const void *to) {
		content->set(to, 0);

		// Notify others about the update. Simply doing 'set' on the content is not enough.
		if (toUpdate)
			toUpdate->set(content);
	}


	DelegatedRef::DelegatedRef(DelegatedCode *owner, code::Ref ref, code::Content *from) :
		Reference(ref, from), owner(owner) {

		moved(address());
	}

	void DelegatedRef::moved(const void *newAddr) {
		owner->moved(newAddr);
	}


	/**
	 * Static engine code.
	 */

	StaticEngineCode::StaticEngineCode(Value result, const void *code) {
		original = new (this) code::RefSource(L"ref-to");
		original->setPtr(code);

		code::Listing *l = redirectCode(result, original);
		code = new (this) code::Binary(engine().arena(), l);
	}

	void StaticEngineCode::newRef() {
		toUpdate->set(code);
	}

#ifdef X86

	code::Listing *StaticEngineCode::redirectCode(Value result, code::Ref ref) {
		using namespace code;

		Listing *l = new (this) Listing();

		if (result.returnInReg()) {
			// The old pointer and the 0 constant will nicely fit into the 'returnData' member.
			*l << push(ptrConst(Offset(0)));
			*l << push(engine().ref(Engine::rEngine));
		} else {
			// The first parameter is, and has to be, a pointer to the returned object.
			*l << mov(ptrA, ptrRel(ptrStack, Offset::sPtr)); // Read the return value ptr.
			*l << push(engine().ref(Engine::rEngine));
			*l << push(ptrA); // Store the return value ptr once more.
		}

		*l << call(ref, result.valType());
		*l << add(ptrStack, ptrConst(Size::sPtr * 2));
		*l << ret(result.valType());

		return l;
	}

#else
#error "Please implement 'redirectCode' for your platform!"
#endif

}
