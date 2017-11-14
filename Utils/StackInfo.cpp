#include "stdafx.h"
#include "StackInfo.h"
#include <typeinfo>

void StackInfo::alloc(StackFrame *frames, nat count) const {}

void StackInfo::free(StackFrame *frames, nat count) const {}

void StackInfo::collect(StackFrame &to, void *frame) const {}

bool StackInfo::format(std::wostream &to, const StackFrame &frame) const {
	return false;
}


StackInfoSet &stackInfo() {
	static StackInfoSet v;
	return v;
}

StackInfoSet::StackInfoSet() {}

int StackInfoSet::attach(const StackInfo &lookup) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		const StackInfo *l = data[i];
		if (!l)
			continue;

		// We assume that if they're of the same type, they're equal.
		if (typeid(*l) == typeid(lookup))
			return -1;
	}

	for (size_t i = 0; i < data.size(); i++) {
		if (!data[i]) {
			data[i] = &lookup;
			return i;
		}
	}

	data.push_back(&lookup);
	return int(data.size() - 1);
}

void StackInfoSet::detach(int id) {
	util::Lock::L z(lock);

	if (id >= 0)
		data[id] = null;
}

void StackInfoSet::alloc(StackFrame *frames, nat count) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		if (data[i])
			data[i]->alloc(frames, count);
	}
}

void StackInfoSet::free(StackFrame *frames, nat count) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		if (data[i])
			data[i]->free(frames, count);
	}
}

void StackInfoSet::collect(StackFrame &to, void *frame) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		if (data[i])
			data[i]->collect(to, frame);
	}
}

void StackInfoSet::format(wostream &to, const StackFrame &frame) {
	util::Lock::L z(lock);

	for (size_t i = 0; i < data.size(); i++) {
		const StackInfo *l = data[i];
		if (!l)
			continue;

		if (l->format(to, frame))
			return;
	}

	// Fallback if all else fails.
	to << L"Unknown function @" << toHex(frame.code);
}

