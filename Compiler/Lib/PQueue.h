#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the template interface for the PQueue<T> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createPQueue(Str *name, ValueArray *params);

	/**
	 * Type for the priority queue.
	 */
	class PQueueType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR PQueueType(Str *name, Type *contents);

		// Parameter.
		Value STORM_FN param() const;

		// Notifications.
		virtual void STORM_FN notifyAdded(NameSet *to, Named *added);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Contained type.
		Type *contents;

		// Add members for types with a '<' operator.
		void addLess();
	};

}
