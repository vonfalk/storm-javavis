#pragma once
#include "Core/Object.h"
#include "Thread.h"

namespace storm {
	namespace debug {
		STORM_PKG(debug);

		/**
		 * Various types used for debugging.
		 */


		/**
		 * Simple linked-list.
		 */
		class Link : public Object {
			STORM_CLASS;
		public:
			nat value;
			Link *next;
		};


		/**
		 * Value-type containing pointers.
		 */
		class Value {
			STORM_VALUE;
		public:
			nat value;
			Link *list;
		};


		/**
		 * Class containing value-types.
		 */
		class ValClass : public Object {
			STORM_CLASS;
		public:
			Value data;
			ValClass *next;
		};


		/**
		 * Simple class with finalizer.
		 *
		 * The destructor prints 'value'.
		 */
		class DtorClass : public Object {
			STORM_CLASS;
		public:
			DtorClass(int v);
			~DtorClass();
			int value;
		};


		/**
		 * Class using pointer-equality in maps.
		 */
		class PtrKey : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			PtrKey();

			// Have we moved recently?
			bool moved() const;

			// Reset moved.
			void reset();

		private:
			size_t oldPos;
		};

	}
}
