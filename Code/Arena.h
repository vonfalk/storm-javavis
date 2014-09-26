#pragma once

#include "Utils/MemoryManager.h"
#include "RefManager.h"
#include "Reference.h"

namespace code {

	// Represents the single root node for all generated code. The arena
	// is (among other things) responsible for memory management and references.
	// This object is supposed to outlive any other objects "owned" by the
	// arena, and will assert otherwise.
	class Arena : NoCopy {
		// These need to access the "refManager" variable.
		friend class RefSource;
		friend class Reference;
		friend class Ref;
	public:
		Arena();
		~Arena();

		// The functions that will be called when the asm instructions
		// addRef and releaseRef are expanded. These are assumed to be
		// constant during the lifetime of the arena. Set them before
		// generating any code.
		// These are assumed to work in pairs, and that a pair of add-release
		// can freely be optimized away.
		typedef void (*RefcountFn)(void *);
		RefcountFn addRef, releaseRef;

		// Allocate and free memory suitable for code.
		void *codeAlloc(nat size);
		void codeFree(void *ptr);

		// Add an external reference.
		Ref external(const String &name, void *ptr);

		template <class T>
		Ref external(const String &name, T *ptr) {
			return external(name, address(ptr));
		}

	private:
		memory::Manager alloc;

		RefManager refManager;

		vector<RefSource *> externalRefs;
	};

}