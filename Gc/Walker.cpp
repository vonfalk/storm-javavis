#include "stdafx.h"
#include "Walker.h"
#include "Format.h"

namespace storm {

	struct PtrScanner {
		typedef int Result;
		typedef PtrWalker *Source;

		PtrScanner(Source &source) : walker(source) {}

		PtrWalker *walker;

		inline ScanOption object(void *start, void *end) { return scanAll; }
		inline bool fix1(void *) { return true; }
		inline Result fix2(void **ptr) {
			walker->exactPointer(ptr);
			return 0;
		}

		inline bool fixHeader1(GcType *) { return true; }
		inline Result fixHeader2(GcType **ptr) {
			walker->header(ptr);
			return 0;
		}
	};


	void PtrWalker::fixed(void *inspect) {
		PtrWalker *me = this;
		fmt::Scan<PtrScanner>::objects(me, inspect, fmt::skip(inspect));
	}

	void PtrWalker::object(RootObject *inspect) {
		PtrWalker *me = this;
		fmt::Scan<PtrScanner>::objects(me, inspect, fmt::skip(inspect));
	}

	void PtrWalker::array(void *inspect) {
		PtrWalker *me = this;
		fmt::Scan<PtrScanner>::objects(me, inspect, fmt::skip(inspect));
	}

}
