#include "stdafx.h"
#include "Debug.h"
#include "Code/VTable.h"
#include "Type.h"
#include "Str.h"
#include "Exception.h"
#include "Code/Debug.h"
#include <iomanip>

namespace storm {

	void append(Par<Str> s, Nat v) {
		s->v += wchar_t(v);
	}

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

	void print(TObject *s) {
		print((Object *)s);
	}

	void printInfo(Object *s) {
		if (s == null) {
			PLN("null");
			return;
		}

		checkLive(s);
		Auto<Str> z = s->toS();
		PLN(z);
		PLN(*s->myType);
		s->myType->vtable.dbg_dump();
	}

	void dumpStack() {
		code::dumpStack();
	}

	void throwError() {
		throw DebugError();
	}

	// Generic object traversal function.
	typedef void (*FoundFn)(void *, TypeVar *, nat, void *);

	// Call 'foundFn' for all objects reachable from 'base'.
	static void traverse(void *base, Type *type, set<Object *> &visited, nat depth, FoundFn fn, void *data) {
		// Examine super class' members.
		if (Type *super = type->super())
			traverse(base, super, visited, depth, fn, data);

		vector<Auto<TypeVar>> vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			TypeVar *v = vars[i].borrow();
			const Value &t = vars[i]->varType;
			Offset offset = vars[i]->offset();

			if (t.ref) {
				WARNING(L"References are not yet supported and therefore ignored!");
			} else if (t.isClass()) {
				// Class.
				Object *o = OFFSET_IN(base, offset.current(), Object *);
				if (o != null && visited.count(o) == 0) {
					visited.insert(o);
					if (fn)
						(*fn)(o, v, depth, data);
					traverse(o, o->myType, visited, depth + 1, fn, data);
				}
			} else if (t.isBuiltIn()) {
				// Built-in type, never a problem. Ignore.
				void *o = &OFFSET_IN(base, offset.current(), void *);
				if (fn)
					(*fn)(o, v, depth, data);
			} else {
				// Value. The value itself is not a problem, but it may contain references that we need to examine.
				void *o = &OFFSET_IN(base, offset.current(), void *);
				if (fn)
					(*fn)(o, v, depth, data);
				traverse(o, t.type, visited, depth + 1, fn, data);
			}
		}
	}

	// Entry point.
	static set<Object *> traverse(Object *o, FoundFn fn, void *data) {
		set<Object *> visited;
		visited.insert(o);
		traverse(o, o->myType, visited, 0, fn, data);
		return visited;
	}


	static void printLayout(void *object, TypeVar *var, nat depth, void *data) {
		const set<Object *> &o = *(const set<Object *>*)data;

		bool hilight = false;
		String value = L"?";
		const Value &v = var->varType;
		if (v.isClass()) {
			Object *obj = (Object *)object;
			value = toS(obj) + L" @" + toHex(obj);
			if (o.count(obj) != 0)
				hilight = true;
		} else if (v.isBuiltIn()) {
			// Built in type.
			if (v.type == intType(var->engine())) {
				value = toS(*(Int *)object);
			} else if (v.type == natType(var->engine())) {
				value = toS(*(Nat *)object);
			}
		} else {
			// Value
			value = L"value:";
		}

		String type = toS(v) + L" = " + var->name;
		if (hilight)
			PNN("->");
		else
			depth++;

		for (nat i = 0; i < depth; i++)
			PNN("  ");
		PLN(type << " " << value);
	}

	// Layout with highlights.
	static void layout(Object *root, const set<Object *> &hilight = set<Object *>()) {
		PLN(root->myType->identifier() << " " << root);
		traverse(root, &printLayout, (void *)&hilight);
	}


	Bool disjoint(Par<Object> a, Par<Object> b) {
		set<Object *> aObjs = traverse(a.borrow(), null, null);
		set<Object *> bObjs = traverse(b.borrow(), null, null);
		set<Object *> same;
		for (set<Object *>::iterator i = aObjs.begin(), end = aObjs.end(); i != end; ++i) {
			if (bObjs.count(*i))
				same.insert(*i);
		}

		if (!same.empty()) {
			PLN("The objects " << a << " and " << b << " are not disjoint!");
			PLN("Reachable from a: " << toS(aObjs));
			PLN("Reachable from b: " << toS(bObjs));
			PLN("Common: " << toS(same));
			PLN("--------- LAYOUT OF A ---------");
			layout(a.borrow(), same);
			PLN("--------- LAYOUT OF B ---------");
			layout(b.borrow(), same);
			return false;
		}

		return true;
	}

	Bool same(Par<Object> a, Par<Object> b) {
		return a.borrow() == b.borrow();
	}

	void layout(Par<Object> obj) {
		layout(obj.borrow());
	}

	void dbgSleep(Int ms) {
		if (ms < 0)
			return;
		Sleep(ms);
	}

	Dbg::Dbg() : v(10) {}

	Dbg::Dbg(Par<Dbg> o) : v(o->v) {}

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

	void Dbg::deepCopy(Par<CloneEnv> e) {
		// Nothing needed here...
	}

	DbgVal::LiveSet DbgVal::live;

	Lock DbgVal::liveLock;

	static void dbg_dumpNolock() {
		for (DbgVal::LiveSet::iterator i = DbgVal::live.begin(); i != DbgVal::live.end(); ++i) {
			PLN(L"Live DbgVal at " << *i);
		}
	}

	void DbgVal::dbg_dump() {
		Lock::L z(liveLock);
		dbg_dumpNolock();
	}

	bool DbgVal::clear() {
		Lock::L z(liveLock);

		if (live.empty())
			return true;

		dbg_dumpNolock();
		return false;
	}

	static void created(DbgVal *v) {
		Lock::L z(DbgVal::liveLock);

		if (DbgVal::live.count(v) != 0) {
			PLN("Trying to re-create a live instance at " << v);
			dbg_dumpNolock();
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
		Lock::L z(liveLock);

		if (live.count(this) == 0) {
			PLN("Trying to assign to a dead object at " << this);
			dbg_dumpNolock();
			assert(false);
		}
		if (live.count(&o) == 0) {
			PLN("Trying to copy from a dead object at " << &o);
			dbg_dumpNolock();
			assert(false);
		}
		v = o.v;
		return *this;
	}

	DbgVal::~DbgVal() {
		Lock::L z(liveLock);

		if (live.count(this) == 0) {
			PLN("Trying to destroy a non-live object at " << this);
			PLN(format(stackTrace()));
			dbg_dumpNolock();
			DebugBreak();
			assert(false);
		}
		live.erase(this);
	}

	void DbgVal::set(Int v) {
		Lock::L z(liveLock);

		if (live.count(this) == 0) {
			PLN("Trying to write to a dead object at " << this);
			dbg_dumpNolock();
			assert(false);
		}
		this->v = v;
	}

	Int DbgVal::get() const {
		Lock::L z(liveLock);

		if (live.count(this) == 0) {
			PLN("Trying to read from a dead object at " << this);
			dbg_dumpNolock();
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

	void DbgVal::deepCopy(Par<CloneEnv> env) {
		// Nothing needed.
	}

	DbgActor::DbgActor() : v(0) {}

	DbgActor::DbgActor(Int v) : v(v) {}

	void DbgActor::set(Int v) {
		this->v = v;
	}

	Int DbgActor::get() const {
		return v;
	}

}
