#include "stdafx.h"
#include "Std.h"
#include "Lib/BuiltIn.h"
#include "Lib/Int.h"
#include "Lib/Bool.h"
#include "Lib/Object.h"
#include "Lib/TObject.h"
#include "Lib/ArrayTemplate.h"
#include "Lib/FutureTemplate.h"
#include "Lib/FnPtrTemplate.h"
#include "Lib/Maybe.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"
#include "NamedThread.h"
#include "TypeCtor.h"
#include "Code/VTable.h"

namespace storm {

	static nat builtInCount() {
		nat count = 0;
		for (const BuiltInType *t = builtInTypes(); t->name; t++)
			count++;
		return count;
	}

	static const BuiltInType *findById(nat id) {
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			if (t->typePtrId == id)
				return t;
		}
		return null;
	}

	static Value createValue(const ValueInnerRef &val, Type *t) {
		Value r(t, (val.options & ValueRef::ref) != ValueRef::nothing);

		if (val.options & ValueRef::array) {
			Type *array = arrayType(t->engine, r);
			r = Value(array);
		}

		if (val.options & ValueRef::maybe) {
			Type *maybe = maybeType(t->engine, r);
			r = Value(maybe);
		}

		return r;
	}

	static Value findInnerValue(const Scope &src, const ValueInnerRef &val, Engine &e) {
		if (val.name == null)
			return Value();

		Auto<Name> name = parseSimpleName(e, val.name);
		Named *f = src.find(name);
		if (Type *t = as<Type>(f))
			return createValue(val, t);

		throw BuiltInError(L"Type " + ::toS(val.name) + L" was not found.");
	}

	static Value findValue(const Scope &src, const ValueRef &val, Engine &e) {
		if (val.options & ValueRef::fnPtr) {
			vector<Value> params;
			params.push_back(findInnerValue(src, val.result, e));
			for (nat i = 0; i < ValueRef::maxParams; i++)
				if (val.params[i].name)
					params.push_back(findInnerValue(src, val.params[i], e));
			return createValue(val, fnPtrType(e, params));
		} else {
			return findInnerValue(src, val, e);
		}
	}


	static void addBuiltIn(Engine &to, const BuiltInFunction *fn, const vector<NamedThread *> &threads, Type *insertInto = null) {
		Named *into = null;
		vector<Value> params;
		bool typeMember = false;

		if (fn->mode & BuiltInFunction::noMember) {
			assert(insertInto == null);

			Auto<Name> pkg = parseSimpleName(to, fn->pkg);
			Package *p = to.package(pkg, true);
			if (!p)
				throw BuiltInError(L"Failed to locate package " + ::toS(fn->pkg));
			into = p;
		} else if (fn->mode & BuiltInFunction::typeMember) {
			assert((fn->mode & BuiltInFunction::hiddenEngine) == 0, L"Not supported with typeMember");
			assert((fn->mode & BuiltInFunction::onThread) == 0, L"Not supported with typeMember");
			Type *t = insertInto;
			if (!t)
				t = to.builtIn(fn->memberId);
			into = t;
			typeMember = true;

			// add the 'this' parameter
			params.push_back(Value::thisPtr(t));
		} else {
			assert(false, L"Unknown function mode. Either typeMember or noMember should be set!");
			return;
		}

		Scope scope(into);
		Value result = findValue(scope, fn->result, to);

		params.reserve(fn->params.size());
		for (nat i = 0; i < fn->params.size(); i++)
			params.push_back(findValue(scope, fn->params[i], to));

		Auto<Function> toAdd;
		if (typeMember && fn->name == Type::DTOR) {
			toAdd = nativeDtor(to, params[0].type, fn->fnPtr);
		} else if (typeMember) {
			// Make sure we handle vtable calls correctly!
			toAdd = nativeMemberFunction(to, params[0].type, result, fn->name, params, fn->fnPtr);
		} else if (fn->mode & BuiltInFunction::hiddenEngine) {
			toAdd = nativeEngineFunction(to, result, fn->name, params, fn->fnPtr);
		} else {
			toAdd = nativeFunction(to, result, fn->name, params, fn->fnPtr);
		}

		if (fn->mode & BuiltInFunction::onThread) {
			toAdd->runOn(threads[fn->threadId]);
		}

		if (typeMember && (fn->mode & BuiltInFunction::virtualFunction) == 0) {
			toAdd->flags |= namedFinal;
		}

		if (Package *p = as<Package>(into)) {
			p->add(toAdd.borrow());
		} else if (Type *t = as<Type>(into)) {
			t->add(toAdd.borrow());
		} else {
			assert(false);
		}
	}

	static void addBuiltIn(Engine &e, const BuiltInVar *var) {
		Type *to = e.builtIn(var->memberId);
		Value type = findValue(*e.scope(), var->type, e);
		Auto<TypeVarCpp> v = CREATE(TypeVarCpp, e, to, type, var->name, Offset(var->offset));
		to->add(v);
	}

	static void addToPkg(Engine &to, const BuiltInType *t) {
		Auto<Name> pkgName = parseSimpleName(to, t->pkg);
		Package *pkg = to.package(pkgName, true);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + ::toS(t->pkg));

		Type *type = to.builtIn(t->typePtrId);
		pkg->add(type);
	}

	static void addHidden(Engine &e, Type *to, nat from, const vector<NamedThread *> &threads) {
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			if (fn->pkg == null && fn->memberId == from) {
				String name = fn->name;
				if (name == L"__ctor" || name == L"__dtor")
					continue;

				// Insert this function into ourselves!
				addBuiltIn(e, fn, threads, to);
			}
		}

		const BuiltInType *p = findById(from);
		if (p->superMode == BuiltInType::superClass)
			addHidden(e, to, p->typePtrId, threads);
	}

	static void addSuper(Engine &to, const BuiltInType *t) {

		Type *tc = to.builtIn(t->typePtrId);
		if (t->superMode == BuiltInType::superClass) {
			// Add as usual.
			Type *super = to.builtIn(t->super);
			if (!super)
				throw BuiltInError(L"Failed to locate super type with id " + ::toS(t->super));
			tc->setSuper(super);
		} else if (tc->typeFlags & typeClass) {
			Type *obj = Object::stormType(to);
			Type *tObj = TObject::stormType(to);

			// We promised to set the super class earlier, so we need some default.
			if (tc == obj || tc == tObj)
				;
			else if (t->superMode == BuiltInType::superThread)
				tc->setSuper(tObj);
			else
				tc->setSuper(obj);
		}
	}

	static NamedThread *addThread(Engine &to, const BuiltInThread *t) {
		Auto<Name> pkgName = parseSimpleName(to, t->pkg);
		Package *pkg = to.package(pkgName, true);
		if (!pkg)
			throw BuiltInError(L"Failed to locate package " + ::toS(t->pkg));

		Thread *thread = t->decl->thread(to);
		Auto<NamedThread> created = CREATE(NamedThread, to, String(t->name), thread);
		pkg->add(created);
		return created.borrow();
	}

	static vector<NamedThread *> addThreads(Engine &to) {
		vector<NamedThread *> threads;
		for (const BuiltInThread *t = builtInThreads(); t->name; t++) {
			NamedThread *created = addThread(to, t);
			threads.push_back(created);
		}
		return threads;
	}

	static void addBuiltIn(Engine &to) {
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addToPkg(to, t);
		}
		vector<NamedThread *> threads = addThreads(to);
		for (const BuiltInFunction *fn = builtInFunctions(); fn->fnPtr; fn++) {
			addBuiltIn(to, fn, threads);
		}
		for (const BuiltInVar *var = builtInVars(); var->name; var++) {
			addBuiltIn(to, var);
		}

		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			switch (t->superMode) {
			case BuiltInType::superHidden:
				// We should copy all the parent functions to ourselves!
				addHidden(to, to.builtIn(t->typePtrId), t->super, threads);
				break;
			case BuiltInType::superThread:
				to.builtIn(t->typePtrId)->setThread(threads[t->super]);
				break;
			}
		}
	}

	void createStdTypes(Engine &to, vector<Auto<Type> > &cached) {
		nat count = builtInCount();
		const BuiltInType *b = builtInTypes();

		cached.resize(count);

		// The first type should be the Type-type, which requires special
		// treatment. (We do not have a Type to feed CREATE yet)
		const BuiltInType *tType = findById(0);
		if (tType == null || tType->name != String(L"Type"))
			throw BuiltInError(L"The first type is not Type.");
		cached[0] = Type::createType(to, L"Type", typeClass | typeManualSuper);

		// Now we can create the rest of all types.
		for (nat i = 0; i < count; i++) {
			const BuiltInType &t = b[i];
			nat id = t.typePtrId;

			if (id == 0)
				continue;

			if (cached.size() <= id)
				cached.resize(id + 1);

			// Here we're not giving the right size for other platforms than the current platform.
			cached[i] = CREATE(Type, to, t.name, TypeFlags(t.typeFlags) | typeManualSuper, Size(t.typeSize), t.cppVTable);
		}
		TODO(L"Find a way to compute the size of types for 64-bit as well!");

		// This should be done first, since package lookup may require the use of as<>.
		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			addSuper(to, t);
		}
	}


	void addStdLib(Engine &to) {
		// Place core types in the core package.
		Auto<Name> coreName = CREATE(Name, to, L"core");
		Package *core = to.package(coreName, true);
		addFnPtrTemplate(core); // needed early.
		addMaybeTemplate(core);
		addArrayTemplate(core);
		core->add(steal(cloneTemplate(to))); // also needed early

		core->add(steal(intType(to)));
		core->add(steal(natType(to)));
		core->add(steal(byteType(to)));
		core->add(steal(boolType(to)));
		core->add(steal(futureTemplate(to)));

		addBuiltIn(to);
	}

	nat maxVTableCount() {
		nat r = 0;

		for (const BuiltInType *t = builtInTypes(); t->name; t++) {
			if (t->typeFlags & typeValue)
				continue;

			if (t->cppVTable == null) {
				WARNING(L"Missing VTable for " << t->name);
			}
			r = max(r, code::vtableCount(t->cppVTable));
		}
		return r;
	}
}
