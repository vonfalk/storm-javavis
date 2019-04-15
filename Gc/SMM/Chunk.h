#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * Description of a piece of allocated memory.
		 */
		class Chunk {
		public:
			Chunk() : at(null), size(0) {}
			Chunk(void *at, size_t size) : at(at), size(size) {}

			// Start of the memory.
			void *at;

			// Size, in bytes.
			size_t size;

			// Empty?
			bool empty() const { return at == null; }
			bool any() const { return at != null; }

			// Contains an address?
			bool has(void *ptr) const {
				return (byte *)at <= (byte *)ptr
					&& (byte *)at + size > (byte *)ptr;
			}

			// Last address (exclusive).
			void *end() const {
				return (byte *)at + size;
			}
		};

		wostream &operator <<(wostream &to, const Chunk &c);

	}
}

#endif
