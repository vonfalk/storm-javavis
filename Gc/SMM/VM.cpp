#include "stdafx.h"
#include "VM.h"

#if STORM_GC == STORM_GC_SMM

#include "VMWin.h"
#include "VMPosix.h"
#include "VMAlloc.h"

namespace storm {
	namespace smm {

		VMAlloc **VM::globalArenas = null;

		size_t VM::usingGlobal = 0;

		util::Lock VM::globalLock;

		VM::VM(VMAlloc *notify, size_t pageSize, size_t granularity)
			: pageSize(pageSize), allocGranularity(granularity), notify(notify) {

			util::Lock::L z(globalLock);
			size_t count = 0;
			if (!globalArenas) {
				initNotify();
			} else {
				while (globalArenas[count])
					count++;
			}

			VMAlloc **n = new VMAlloc*[count + 2];
			for (size_t i = 0; i < count; i++)
				n[i] = globalArenas[i];
			n[count] = notify;
			n[count + 1] = null; // Mark the end.

			VMAlloc **old = globalArenas;
			atomicWrite(globalArenas, n);

			// Wait for anyone who might know about the old value to be done.
			while (atomicRead(usingGlobal) != 0)
				;
			delete []old;
		}

		VM::~VM() {
			util::Lock::L z(globalLock);
			size_t count = 0;
			while (globalArenas[count])
				count++;

			VMAlloc **old = globalArenas;
			VMAlloc **n = null;
			if (count > 1) {
				n = new VMAlloc*[count + 1];

				bool found = false;
				size_t to = 0;
				for (size_t i = 0; i < count; i++) {
					if (!found && globalArenas[i] == notify)
						found = true;
					else
						n[to++] = globalArenas[i];
				}

				assert(found, L"We didn't find the VMAlloc we're supposed to notify, something is very wrong!");

				n[to++] = null;
			}

			// Publish our new result.
			atomicWrite(globalArenas, n);

			// Wait for anyone who might know about the old value to be done.
			while (atomicRead(usingGlobal) != 0)
				;
			delete []old;

			if (count == 1) {
				destroyNotify();
			}
		}

		bool VM::notifyWrite(void *addr) {
			bool any = false;

			atomicIncrement(usingGlobal);
			VMAlloc **table = atomicRead(globalArenas);

			if (table) {
				for (; *table; table++) {
					any |= (*table)->onProtectedWrite(addr);
				}
			}

			atomicDecrement(usingGlobal);
			return any;
		}


#if defined(WINDOWS)

		VM *VM::create(VMAlloc *alloc) {
			return VMWin::create(alloc);
		}

		void VM::initNotify() {
			VMWin::initNotify();
		}

		void VM::destroyNotify() {
			VMWin::destroyNotify();
		}

#elif defined(POSIX)

		VM *VM::create(VMAlloc *alloc) {
			return VMPosix::create(alloc);
		}

		void VM::initNotify() {
			VMPosix::initNotify();
		}

		void VM::destroyNotify() {
			VMPosix::destroyNotify();
		}

#else
#error "Please implement the virtual memory functionality for your platform!"
#endif

	}
}

#endif
