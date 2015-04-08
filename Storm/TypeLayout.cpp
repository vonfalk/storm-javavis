#include "stdafx.h"
#include "TypeLayout.h"
#include "Exception.h"
#include "Utils/MapSwap.h"
#include <iomanip>

namespace storm {

	void TypeLayout::add(Named *n) {
		if (TypeVar *v = as<TypeVar>(n))
			add(v);
	}

	void TypeLayout::add(TypeVar *v) {
		assert(offsets.count(v) == 0, "Variable already inserted!");

		if (TypeVarCpp *cppVar = as<TypeVarCpp>(v)) {
			// Add it just as it is. We will not read it anyway...
			offsets.insert(make_pair(v, Size()));
		} else {
			// Allocate 'v' at the end.
			Size vSz = v->varType.size();
			offsets.insert(make_pair(v, total + vSz.align()));
			total += vSz;
		}
	}

	Offset TypeLayout::offset(Size parentSize, const TypeVar *v) const {
		// Sad, but needed...
		TypeVar *key = const_cast<TypeVar *>(v);
		if (TypeVarCpp *k = as<TypeVarCpp>(key)) {
			return k->myOffset;
		}

		OffsetMap::const_iterator f = offsets.find(key);
		if (f == offsets.end()) {
			assert(false, "Variable not added here!");
			throw InternalError(L"Variable not added properly: " + ::toS(v));
		}

		Size r = total.align() + f->second;
		return Offset(parentSize + r);
	}

	Size TypeLayout::size(Size parentSize) const {
		return parentSize + total;
	}

	bool TypeLayout::operator ==(const TypeLayout &o) const {
		if (total != o.total)
			return false;

		OffsetMap::const_iterator i = offsets.begin(), e = offsets.end();
		for (; i != e; ++i) {
			OffsetMap::const_iterator oi = o.offsets.find(i->first);
			if (oi == o.offsets.end())
				return false;
			if (oi->second != i->second)
				return false;
		}

		return true;
	}

	bool TypeLayout::operator !=(const TypeLayout &o) const {
		return !(*this == o);
	}

	void TypeLayout::format(wostream &o, Size rel) const {
		o << L"Layout " << total << L" bytes";
		if (rel != Size())
			o << L", offset: " << rel;
		o << endl;

		Indent indent(o);

		map<Size, TypeVar *> sorted;
		mapSwap(offsets, sorted);

		Size align = total.align();
		for (map<Size, TypeVar *>::iterator i = sorted.begin(); i != sorted.end(); ++i) {
			Offset s = offset(rel, i->second);
			o << std::setw(10) << i->second->name << L": " << ::toS(s) << endl;
		}
	}

	void TypeLayout::output(wostream &o) const {
		format(o);
	}

	vector<Auto<TypeVar> > TypeLayout::variables() const {
		vector<Auto<TypeVar> > r;
		r.reserve(offsets.size());
		for (OffsetMap::const_iterator i = offsets.begin(); i != offsets.end(); ++i) {
			r.push_back(capture(i->first));
		}
		return r;
	}

}
