#include "stdafx.h"
#include "Debug.h"
#include "Code/VTable.h"
#include "Type.h"
#include "Str.h"
#include "Exception.h"
#include "Code/Debug.h"

namespace storm {

	void dbgBreak() {
		DebugBreak();
	}

	void printVTable(Object *obj) {
		checkLive(obj);
		void *v = code::vtableOf(obj);
		PLN("Vtable of: " << obj << " is " << v);
		PLN(" Refs: " << obj->dbg_refs());
	}

	void ptrace(Int z) {
		PLN("At: " << z);
	}

	void print(Object *s) {
		if (s == null) {
			PLN("null");
			return;
		}

		checkLive(s);

		Auto<Str> z = s->toS();
		PLN(z);
	}

	void dumpStack() {
		code::dumpStack();
	}

	void throwError() {
		throw InternalError(L"Someone called 'throwError'.");
	}


	Dbg::Dbg() : v(10) {}

	Dbg::Dbg(Int v) : v(v) {}

	Dbg::~Dbg() {}

	void Dbg::set(Int v) {
		this->v = v;
	}

	Int Dbg::get() {
		return v;
	}

	void Dbg::dbg() {
		PLN("Debug object. Type: " << myType->identifier() << " value: " << v);
	}

	Int Dbg::returnOne() {
		return 1;
	}

	Int Dbg::returnTwo() {
		return 2;
	}

	DbgVal::LiveSet DbgVal::live;

	void DbgVal::dbg_dump() {
		for (LiveSet::iterator i = live.begin(); i != live.end(); ++i) {
			PLN(L"Live DbgVal at " << *i);
		}
	}

	bool DbgVal::clear() {
		if (live.empty())
			return true;

		dbg_dump();
		return false;
	}

	static void created(DbgVal *v) {
		if (DbgVal::live.count(v) != 0) {
			PLN("Trying to re-create a live instance at " << v);
			DbgVal::dbg_dump();
			assert(false);
		}
		DbgVal::live.insert(v);
	}

	DbgVal::DbgVal() : v(10) {
		created(this);
	}

	DbgVal::DbgVal(Int v) : v(v) {
		created(this);
	}

	DbgVal::DbgVal(const DbgVal &o) : v(o.get()) {
		created(this);
	}

	DbgVal &DbgVal::operator =(const DbgVal &o) {
		if (live.count(this) == 0) {
			PLN("Trying to assign to a dead object at " << this);
			dbg_dump();
			assert(false);
		}
		if (live.count(&o) == 0) {
			PLN("Trying to copy from a dead object at " << &o);
			dbg_dump();
			assert(false);
		}
		v = o.v;
		return *this;
	}

	DbgVal::~DbgVal() {
		if (live.count(this) == 0) {
			PLN("Trying to destroy a non-live object at " << this);
			dbg_dump();
			assert(false);
		}
		live.erase(this);
	}

	void DbgVal::set(Int v) {
		if (live.count(this) == 0) {
			PLN("Trying to write to a dead object at " << this);
			dbg_dump();
			assert(false);
		}
		this->v = v;
	}

	Int DbgVal::get() const {
		if (live.count(this) == 0) {
			PLN("Trying to read from a dead object at " << this);
			dbg_dump();
			assert(false);
		}
		return v;
	}

	bool DbgVal::operator ==(const DbgVal &o) const {
		return get() == o.get();
	}

	wostream &operator <<(wostream &to, const DbgVal &v) {
		return to << L"DbgVal:" << v.get();
	}

}
