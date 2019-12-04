#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Config.h"
#include "Scanner.h"
#include "Generation.h"

namespace storm {
	namespace smm {

		/**
		 * Scanners for updating forwarders.
		 */


		/**
		 * Update all forwarders in a particular generation.
		 */
		struct UpdateFwd {
			typedef int Result;
			typedef const Generation::State Source;

			Arena &arena;
			byte id;

			UpdateFwd(const Generation::State &state) : arena(state.arena()), id(state.identifier()) {}

			inline bool fix1(void *ptr) const {
				return arena.memGeneration(ptr) == id;
			}

			inline Result fix2(void **ptr) const {
				fmt::Obj *o = fmt::fromClient(*ptr);
				// This will not update 'ptr' if it isn't a forwarding object.
				objIsFwd(o, ptr);
				return 0;
			}

			SCAN_FIX_HEADER
		};


		/**
		 * Update all forwarders for weak references. We assume we will only be executed on weak references.
		 */
		struct UpdateWeakFwd {
			typedef int Result;
			typedef const Generation::State Source;

			Arena &arena;
			byte id;
			const Generation::State &state;

			UpdateWeakFwd(const Generation::State &state)
				: arena(state.arena()), id(state.identifier()), state(state) {}

			inline bool fix1(void *ptr) const {
				return arena.memGeneration(ptr) == id;
			}

			inline Result fix2(void **ptr) const {
				fmt::Obj *o = fmt::fromClient(*ptr);
				// Note: objIsFwd will update 'ptr' if it is a forwarder!
				if (objIsFwd(o, ptr)) {
					// Make sure it's not pointing to the finalizer queue now...
					if (arena.memGeneration(*ptr) == finalizerIdentifier)
						*ptr = null;
				} else if (!state.isPinned(*ptr, fmt::objSkip(o))) {
					// If it is not pinned, it will perish.
					*ptr = null;
				}
				return 0;
			}

			SCAN_FIX_HEADER
		};


		/**
		 * Update a set of objects that are a mix of weak and non-weak objects. Assumes that the
		 * predicate is called before each object is scanned.
		 */
		struct MixedPredicate {
			const Generation::State &state;
			mutable bool isWeak;

			MixedPredicate(const Generation::State &state) : state(state), isWeak(false) {}

			inline fmt::ScanOption operator()(void *obj, void *) const {
				isWeak = fmt::objIsWeak(fmt::fromClient(obj));
				return fmt::scanAll;
			}
		};

		struct UpdateMixedFwd {
			typedef int Result;
			typedef MixedPredicate Source;

			Arena &arena;
			byte id;
			const Generation::State &state;
			const MixedPredicate &predicate;

			UpdateMixedFwd(const MixedPredicate &predicate)
				: arena(predicate.state.arena()), id(predicate.state.identifier()),
				  state(predicate.state), predicate(predicate) {}

			inline bool fix1(void *ptr) const {
				return arena.memGeneration(ptr) == id;
			}

			inline Result fix2(void **ptr) const {
				// The common path...
				fmt::Obj *o = fmt::fromClient(*ptr);
				bool wasFwd = objIsFwd(o, ptr);

				if (predicate.isWeak) {
					if (wasFwd) {
						if (arena.memGeneration(*ptr) == finalizerIdentifier)
							*ptr = null;
					} else if (!state.isPinned(*ptr, fmt::objSkip(o))) {
						*ptr = null;
					}
				}

				return 0;
			}

			inline bool fixHeader1(GcType *header) { return fix1(header); }
			inline Result fixHeader2(GcType **header) {
				void **ptr = (void **)header;

				// We should always scan the header!
				fmt::Obj *o = fmt::fromClient(*ptr);
				bool wasFwd = objIsFwd(o, ptr);

				if (wasFwd) {
					if (arena.memGeneration(*ptr) == finalizerIdentifier)
						*ptr = null;
				} else if (!state.isPinned(*ptr, fmt::objSkip(o))) {
					*ptr = null;
				}

				return 0;
			}
		};

	}
}

#endif
