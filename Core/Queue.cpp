#include "stdafx.h"
#include "Queue.h"

namespace storm {

	QueueBase::QueueBase(const Handle &type) : handle(type), data(null), head(0) {}

	QueueBase::QueueBase(const QueueBase &other) : handle(other.handle), data(null), head(0) {
		if (other.empty())
			return;

		ensure(other.count());

		Nat at = other.head;
		for (Nat i = 0; i < data->filled; i++, step(at)) {
			handle.safeCopy(ptr(i), other.ptr(at));
		}
		data->filled = other.count();
	}

	void QueueBase::deepCopy(CloneEnv *env) {
		if (handle.deepCopyFn && data) {
			Nat at = head;
			for (Nat i = 0; i < data->filled; i++, step(at)) {
				(*handle.deepCopyFn)(ptr(at), env);
			}
		}
	}

	void QueueBase::ensure(Nat n) {
		if (n == 0)
			return;

		Nat oldCap = data ? data->count : 0;
		if (oldCap >= n)
			return;

		Nat oldCount = count();

		Nat newCap = max(Nat(16), oldCap * 2);
		if (newCap < n)
			newCap = n;
		GcArray<byte> *newData = runtime::allocArray<byte>(engine(), handle.gcArrayType, newCap);

		if (data) {
			// Copy from head to the end.
			Nat firstBatch = min(oldCount, data->count - head);
			memcpy(ptr(newData, 0), ptr(head), handle.size*firstBatch);

			// If needed, copy remaining elements.
			if (firstBatch < oldCount)
				memcpy(ptr(newData, firstBatch), ptr(0), handle.size*(oldCount - firstBatch));

			// Update metadata.
			data->filled = 0;
			newData->filled = oldCount;
		}

		// Swap contents.
		data = newData;
		head = 0;
	}

	void QueueBase::reserve(Nat n) {
		ensure(n);
	}

	void QueueBase::clear() {
		data = null;
		head = 0;
	}

	void *QueueBase::topRaw() {
		if (empty())
			throw new (this) QueueError(S("Cannot get the top element of an empty queue."));

		return ptr(head);
	}

	void QueueBase::pop() {
		if (empty())
			throw new (this) QueueError(S("Cannot pop an empty queue."));

		head++;
		if (head >= data->count)
			head -= data->count;
		data->filled--;
	}

	void QueueBase::pushRaw(const void *elem) {
		ensure(count() + 1);

		Nat to = head + data->filled;
		if (to >= data->count)
			to -= data->count;
		handle.safeCopy(ptr(to), elem);
		data->filled++;
	}

	void QueueBase::toS(StrBuf *to) const {
		*to << S("Queue:[");
		for (Nat at = head, i = 0; i < count(); i++, step(at)) {
			if (i > 0)
				*to << S(", ");
			(*handle.toSFn)(ptr(at), to);
		}
		*to << S("]");
	}


	QueueError::QueueError(const wchar *msg) : msg(new (engine()) Str(msg)) {
		saveTrace();
	}

	QueueError::QueueError(Str *msg) : msg(msg) {
		saveTrace();
	}

	void QueueError::message(StrBuf *to) const {
		*to << S("Queue error: ") << msg;
	}

}
