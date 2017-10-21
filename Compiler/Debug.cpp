#include "stdafx.h"
#include "Debug.h"
#include "Exception.h"
#include "Variable.h"
#include "Type.h"
#include "Core/Str.h"
#include "Code/Debug.h"
#include "Utils/Memory.h"

namespace storm {
	namespace debug {

		Link *createList(EnginePtr ep, Nat n) {
			Link *start = null;
			Link *prev = null;
			Engine &e = ep.v;

			for (nat i = 0; i < n; i++) {
				Link *now = new (e) Link;
				now->value = i;

				if (prev == null) {
					start = now;
				} else {
					prev->next = now;
				}
				prev = now;
			}

			return start;
		}

		Bool checkList(Link *first, Nat n) {
			Link *at = first;
			for (nat i = 0; i < n; i++) {
				if (!at)
					return false;
				if (at->value != i)
					return false;

				at = at->next;
			}

			return at == null;
		}


		Value::Value() : value(0), list(null) {}

		DtorClass::DtorClass(int v) : value(v) {}

		DtorClass::~DtorClass() {
			PVAR(value);
		}

		PtrKey::PtrKey() {
			reset();
		}

		void PtrKey::reset() {
			oldPos = size_t(this);
		}

		bool PtrKey::moved() const {
			return size_t(this) != oldPos;
		}

		Extend::Extend() : v(1) {}

		Extend::Extend(Int v) : v(v) {}

		Int Extend::value() {
			return v;
		}

		void dbgBreak() {
#ifdef WINDOWS
			DebugBreak();
#else
			abort();
#endif
		}

		Float dbgFloat() {
			return 23.0f;
		}

		void print(Object *s) {
			static util::Lock lock;
			util::Lock::L w(lock);

			if (s == null) {
				PLN("null");
				return;
			}

			PLN(s->toS());
		}

		void print(TObject *s) {
			// Make sure to execute on the right thread!
			const os::Thread &target = s->thread->thread();
			if (os::Thread::current() != target) {
				os::Future<void> fut;
				os::FnCall<void, 1> p = os::fnCall().add(s);
				typedef void (*PrintFn)(Object *);
				os::UThread::spawn(address((PrintFn)&print), false, p, fut, &target);
				fut.result();
			} else {
				print((Object *)s);
			}
		}

		void dumpStack() {
			code::dumpStack();
		}

		void throwError() {
			throw DebugError();
		}

		// Generic object traversal function.
		typedef void (*FoundFn)(void *, MemberVar *, nat, void *);

		// Call 'foundFn' for all objects reachable from 'base'.
		static void traverse(void *base, Type *type, set<Object *> &visited, nat depth, FoundFn fn, void *data) {
			// Examine super class' members.
			if (Type *super = type->super())
				traverse(base, super, visited, depth, fn, data);

			Array<MemberVar *> *vars = type->variables();
			for (nat i = 0; i < vars->count(); i++) {
				MemberVar *v = vars->at(i);
				const storm::Value &t = v->type;
				Offset offset = v->offset();

				if (t.ref) {
					WARNING(L"References are not yet supported and therefore ignored!");
				} else if (t.isClass()) {
					// Class.
					Object *o = OFFSET_IN(base, offset.current(), Object *);
					if (o != null && visited.count(o) == 0) {
						visited.insert(o);
						if (fn)
							(*fn)(o, v, depth, data);
						traverse(o, runtime::typeOf(o), visited, depth + 1, fn, data);
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
			TODO(L"Use a Storm set here instead!");
			set<Object *> visited;
			visited.insert(o);
			traverse(o, runtime::typeOf(o), visited, 0, fn, data);
			return visited;
		}


		static void printLayout(void *object, MemberVar *var, nat depth, void *data) {
			const set<Object *> &o = *(const set<Object *>*)data;

			bool hilight = false;
			String value = L"?";
			const storm::Value &v = var->type;
			if (v.isClass()) {
				Object *obj = (Object *)object;
				value = ::toS(obj) + L" @" + toHex(obj);
				if (o.count(obj) != 0)
					hilight = true;
			} else if (v.isBuiltIn()) {
				// Built in type.
				if (v.type == StormInfo<Int>::type(var->engine())) {
					value = ::toS(*(Int *)object);
				} else if (v.type == StormInfo<Nat>::type(var->engine())) {
					value = ::toS(*(Nat *)object);
				}
			} else {
				// Value
				value = L"value:";
			}

			String type = ::toS(v) + L" = " + ::toS(var->name);
			if (hilight)
				PNN("->");
			else
				depth++;

			for (nat i = 0; i < depth; i++)
				PNN("  ");
			PLN(type << " " << value);
		}

		// Layout with highlights.
		static void layout(Object *root, const set<Object *> &hilight) {
			PLN(runtime::typeOf(root)->identifier() << " " << root);
			traverse(root, &printLayout, (void *)&hilight);
		}

		void layout(Object *obj) {
			layout(obj, set<Object *>());
		}

		Bool disjoint(Object *a, Object *b) {
			set<Object *> aObjs = traverse(a, null, null);
			set<Object *> bObjs = traverse(b, null, null);
			set<Object *> same;
			for (set<Object *>::iterator i = aObjs.begin(), end = aObjs.end(); i != end; ++i) {
				if (bObjs.count(*i))
					same.insert(*i);
			}

			if (!same.empty()) {
				PLN("The objects " << a << " and " << b << " are not disjoint!");
				PLN("Reachable from a: " << ::toS(aObjs));
				PLN("Reachable from b: " << ::toS(bObjs));
				PLN("Common: " << ::toS(same));
				PLN("--------- LAYOUT OF A ---------");
				layout(a, same);
				PLN("--------- LAYOUT OF B ---------");
				layout(b, same);
				return false;
			}

			return true;
		}

		Bool same(Object *a, Object *b) {
			return a == b;
		}

		void dbgSleep(Int ms) {
			if (ms < 0)
				return;
#ifdef WINDOWS
			Sleep(ms);
#else
			usleep(useconds_t(ms) * 1000);
#endif
		}

		void dbgYield() {
			os::UThread::leave();
		}

		Dbg::Dbg() : v(10) {}

		Dbg::Dbg(Dbg *o) : v(o->v) {}

		Dbg::Dbg(Int v) : v(v) {}

		Dbg::~Dbg() {}

		void Dbg::set(Int v) {
			this->v = v;
		}

		Int Dbg::get() {
			return v;
		}

		DbgVal Dbg::asDbgVal() {
			return DbgVal(v);
		}

		void Dbg::dbg() {
			PLN("Debug object. Type: " << runtime::typeOf(this) << " value: " << v);
		}

		Int Dbg::returnOne() {
			return 1;
		}

		Int Dbg::returnTwo() {
			return 2;
		}

		void Dbg::deepCopy(CloneEnv *e) {
			// Nothing needed here...
		}

		DbgVal::LiveSet DbgVal::live;

		os::Lock DbgVal::liveLock;

		static void dbg_dumpNolock() {
			for (DbgVal::LiveSet::iterator i = DbgVal::live.begin(); i != DbgVal::live.end(); ++i) {
				PLN(L"Live DbgVal at " << *i << L" (" << (*i)->v << L")");
			}
		}

		void DbgVal::dbg_dump() {
			os::Lock::L z(liveLock);
			dbg_dumpNolock();
		}

		bool DbgVal::clear() {
			os::Lock::L z(liveLock);

			if (live.empty())
				return true;

			dbg_dumpNolock();
			return false;
		}

		static void created(DbgVal *v) {
			os::Lock::L z(DbgVal::liveLock);

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
			os::Lock::L z(liveLock);

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
			os::Lock::L z(liveLock);

			if (live.count(this) == 0) {
				PLN("Trying to destroy a non-live object at " << this);
				PLN(format(stackTrace()));
				dbg_dumpNolock();
				assert(false);
			}
			live.erase(this);
		}

		void DbgVal::set(Int v) {
			os::Lock::L z(liveLock);

			if (live.count(this) == 0) {
				PLN("Trying to write to a dead object at " << this);
				dbg_dumpNolock();
				assert(false);
			}
			this->v = v;
		}

		Int DbgVal::get() const {
			os::Lock::L z(liveLock);

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

		void DbgVal::deepCopy(CloneEnv *env) {
			// Nothing needed.
		}

		Str *toS(EnginePtr e, DbgVal v) {
			return new (e.v) Str(::toS(v).c_str());
		}

		DbgActor::DbgActor() : v(0) {}

		DbgActor::DbgActor(Int v) : v(v) {}

		void DbgActor::set(Int v) {
			this->v = v;
		}

		Int DbgActor::get() const {
			return v;
		}

		Str *DbgActor::echo(Str *v) {
			return v;
		}

		DbgVal DbgActor::asDbgVal() {
			return DbgVal(v);
		}


		DbgNoToS::DbgNoToS() : dummy(0) {}

	}
}

