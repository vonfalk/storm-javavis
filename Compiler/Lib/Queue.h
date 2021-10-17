#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	// Create types for unknown implementations.
	Type *createQueue(Str *name, ValueArray *params);

	/**
	 * Type for queues.
	 */
	class QueueType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR QueueType(Str *name, Type *contents);

		// Late init.
		virtual void lateInit();

		// Parameter.
		Value STORM_FN param() const;

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;
	};

	/**
	 * Iterator type for queues.
	 */
	class QueueIterType : public Type {
		STORM_CLASS;
	public:
		// Create.
		QueueIterType(Type *param);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;
	};


	Bool STORM_FN isQueue(Value v);
	Value STORM_FN unwrapQueue(Value v);
	Value STORM_FN wrapQueue(Value v);

}
