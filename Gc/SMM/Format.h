#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Gc/Format.h"

namespace storm {
	namespace fmt {

		/**
		 * Our additions to the standard format.
		 *
		 * Since we have a different approach to finalization compared to e.g. the MPS, we don't
		 * need the 'finalized' flag available for all objects in the format. Rather, we use this
		 * flag to indicate if an object needs finalization or not. This makes it really fast to
		 * check if a particular object needs finalization or not, as that information is inlined in
		 * the header pointer.
		 */

		// Indicate that the object has a finalizer.
		static inline void objSetHasFinalizer(Obj *obj) {
			objSetFinalized(obj);
		}
		static inline void setHasFinalizer(void *obj) {
			objSetHasFinalizer(fromClient(obj));
		}

		// Check if the object has a finalizer.
		static inline bool objHasFinalizer(const Obj *obj) {
			return objIsFinalized(obj);
		}
		static inline bool hasFinalizer(const void *obj) {
			return objHasFinalizer(fromClient(obj));
		}

		// Call the finalizer of an object.
		static inline void objFinalize(Obj *obj) {
			if (objIsSpecial(obj))
				return;

			void *client = toClient(obj);
			if (objIsCode(obj)) {
				code::finalize(client);
				return;
			}

			const GcType &h = objHeader(obj)->obj;
			if (h.finalizer) {
				// Make sure the object was initialized properly!
				bool finalize = true;
				if (h.kind == GcType::tFixedObj || h.kind == GcType::tType)
					finalize = vtable::from((RootObject *)client) != null;

				if (finalize) {
					typedef void (*Fn)(void *);
					Fn fn = (Fn)h.finalizer;
					(*fn)(client);
				}
			}
		}

	}
}

#endif
