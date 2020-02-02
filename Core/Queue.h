#pragma once
#include "Object.h"
#include "Handle.h"
#include "GcArray.h"
#include "StrBuf.h"
#include "Exception.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Base class for queues.
	 */
	class QueueBase : public Object {
		STORM_CLASS;
	public:
		// Empty queue.
		QueueBase(const Handle &type);

		// Copy another queue.
		QueueBase(const QueueBase &other);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Reserve size.
		void STORM_FN reserve(Nat count);

		// Clear.
		void STORM_FN clear();

		// Number of elements.
		inline Nat STORM_FN count() const { return data ? data->filled : 0; }

		// Any elements?
		inline Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Get the top element.
		void *CODECALL topRaw();

		// Pop an element.
		void STORM_FN pop();

		// Push an element.
		void CODECALL pushRaw(const void *elem);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Handle of the contained type.
		const Handle &handle;

	private:
		// Data. We store 'count' in here.
		GcArray<byte> *data;

		// Head element.
		Nat head;

		// Get the pointer to an element.
		inline void *ptr(GcArray<byte> *data, Nat id) const { return data->v + (id * handle.size); }
		inline void *ptr(Nat id) const { return ptr(data, id); }

		// Ensure data contains at least 'n' elements.
		void ensure(Nat n);

		// Step a variable around in the buffer.
		inline void step(Nat &at) const {
			if (++at == data->count)
				at = 0;
		}
	};

	// Declare the template in Storm.
	STORM_TEMPLATE(Queue, createQueue);

	/**
	 * Class used from C++.
	 */
	template <class T>
	class Queue : public QueueBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, QueueId, 1, StormInfo<T>::id());
		}

		// Empty queue.
		Queue() : QueueBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy array.
		Queue(const Queue &o) : QueueBase(o) {
			runtime::setVTable(this);
		}

		// Get the top element.
		T &top() {
			return *(T *)topRaw();
		}

		// Push element.
		void push(const T &item) {
			pushRaw(&item);
		}

		// Push.
		Queue<T> &operator <<(const T &item) {
			pushRaw(&item);
			return *this;
		}
	};

	/**
	 * Custom error type for queues.
	 */
	class EXCEPTION_EXPORT QueueError : public Exception {
		STORM_EXCEPTION;
	public:
		QueueError(const wchar *msg);
		STORM_CTOR QueueError(Str *msg);
		virtual void STORM_FN message(StrBuf *to) const;
	private:
		MAYBE(Str *) msg;
	};

}
