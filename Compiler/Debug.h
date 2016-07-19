#pragma once
#include "Object.h"

namespace storm {
	namespace debug {

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

	}
}
