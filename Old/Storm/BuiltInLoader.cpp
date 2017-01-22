#include "stdafx.h"
#include "BuiltInLoader.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"
#include "Function.h"
#include "TypeCtor.h"
#include "Code/VTable.h"
#include "Lib/Maybe.h"
#include "Shared/Array.h"
#include "Lib/FnPtrTemplate.h"
#include "Lib/FutureTemplate.h"

namespace storm {

	static nat numTypes(const BuiltIn &src) {
		nat count = 0;
		for (const BuiltInType *t = src.types; t->name; t++)
			count++;
		return count;
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

		Auto<SimpleName> name = parseSimpleName(e, val.name);
		Auto<Named> f = src.find(name);
		if (Type *t = as<Type>(f.borrow()))
			return createValue(val, t);

		throw BuiltInError(L"Type " + String(val.name) + L" was not found.");
	}

	static Value findValue(const Scope &scope, const ValueRef &val, Engine &e) {
		if (val.options & ValueRef::fnPtr) {
			vector<Value> params;
			params.push_back(findInnerValue(scope, val.result, e));
			for (nat i = 0; i < ValueRef::maxParams; i++)
				if (val.params[i].name)
					params.push_back(findInnerValue(scope, val.params[i], e));
			return createValue(val, fnPtrType(e, params));
		} else {
			return findInnerValue(scope, val, e);
		}
	}

	// Create a type (or find it if it is referring to an external type).
	static Type *createType(Engine &e, const BuiltInType &t) {
		if (t.superMode == BuiltInType::superExternal) {
			// External types have an absolute name!
			Auto<SimpleName> name = parseSimpleName(e, t.pkg);
			name->add(steal(CREATE(SimplePart, e, t.name)));
			Auto<Type> t = steal(e.scope()->find(name)).as<Type>();
			if (!t)
				throw BuiltInError(L"The external type " + ::toS(name) + L" was not found.");
			return t.ret();
		} else {
			Size size(t.typeSize); // Note: this is not correct for anything but the current platform!
			return CREATE(Type, e, t.name, TypeFlags(t.typeFlags) | typeManualSuper, size, t.cppVTable);
		}
	}

	// Set the super type of a type. Does not set threads to threaded types.
	static void setSuper(Engine &e, const BuiltInLoader::TypeList &l, const BuiltInType &t) {
		Par<Type> type = l[t.typePtrId];

		switch (t.superMode) {
		case BuiltInType::superExternal:
			// Nothing needs to be done.
			break;
		case BuiltInType::superClass:
			if (Type *super = l[t.super].borrow())
				type->setSuper(super);
			else
				throw BuiltInError(L"Failed to locate super type with id " + ::toS(t.super));
			break;
		case BuiltInType::superThread:
			assert(type->typeFlags & typeClass, L"Value types can not have an associated thread!");
			if (type->typeFlags & typeClass)
				type->setSuper(TObject::stormType(e));
			break;
		default:
			// We promised to set a super class, so we need to do that...
			if (type->typeFlags & typeClass) {
				if (type != Object::stormType(e) &&
					type != TObject::stormType(e))
					type->setSuper(Object::stormType(e));
			}
			break;
		}
	}

	/**
	 * Public API
	 */

	BuiltInLoader::BuiltInLoader(Engine &e, TypeList &to, const BuiltIn &from, Package *pkg) :
		e(e), types(to), src(from), root(pkg) {}

	void BuiltInLoader::setRoot(Package *pkg) {
		assert(root == null);
		root = pkg;
	}

	NameSet *BuiltInLoader::findNameSet(const String &pkg) {
		Auto<SimpleName> pkgName = parseSimpleName(e, pkg);
		NameSet *r = e.nameSet(root, pkgName, true);
		if (!r)
			throw BuiltInError(L"Failed to locate package or type " + ::toS(pkg));

		return r;
	}

	Package *BuiltInLoader::findPkg(const String &pkg) {
		Auto<SimpleName> pkgName = parseSimpleName(e, pkg);
		Package *r = e.package(root, pkgName, true);
		if (!r)
			throw BuiltInError(L"Failed to locate package " + ::toS(pkg));

		return r;
	}

	nat BuiltInLoader::vtableCapacity() {
		nat r = 0;

		for (nat i = 0; src.types[i].name; i++) {
			const BuiltInType &t = src.types[i];
			if (t.typeFlags & typeValue)
				continue;
			if (t.superMode == BuiltInType::superExternal)
				continue;

			if (t.cppVTable == null) {
				WARNING(L"Missing VTable for " << t.name);
			}
			r = max(r, code::vtableCount(t.cppVTable));
		}

		return r;
	}


	void BuiltInLoader::loadThreads() {
		threads.clear();

		for (nat i = 0; src.threads[i].name; i++) {
			const BuiltInThread &t = src.threads[i];
			NameSet *into = findNameSet(t.pkg);
			Thread *thread = t.decl->thread(e);
			Auto<NamedThread> created = CREATE(NamedThread, e, String(t.name), thread);
			into->add(created);
			e.threadName(t.decl->identifier(), created);
			threads.push_back(created);
		}
	}

	void BuiltInLoader::loadTypes() {
		createTypes();
		finalizeTypes();
	}

	void BuiltInLoader::createTypes() {
		nat count = numTypes(src);
		types.resize(count);

		for (nat i = 0; i < count; i++) {
			const BuiltInType &t = src.types[i];
			nat id = t.typePtrId; // TODO: Should be equal to i, remove it!
			assert(i == id);

			// Already initialized?
			if (types[i]) {
				assert(types[i]->name == t.name);
				continue;
			}

			types[i] = createType(e, t);
		}
		TODO(L"Find a way to compute the size of types for 64-bit as well!");

		// Set up super classes so that as<> works. We can not do that inside the above loop since
		// super classes have not neccessarily been created before derived classes.
		for (nat i = 0; i < count; i++) {
			setSuper(e, types, src.types[i]);
		}

		// Now, we can check which types require us to copy members.
		for (nat i = 0; i < count; i++) {
			const BuiltInType &t = src.types[i];
			if (t.superMode == BuiltInType::superHidden) {
				Type *to = types[i].borrow();
				for (Type *from = types[t.super].borrow(); from; from = from->super()) {
					copyMembers[from].push_back(to);
				}
			}
		}
	}

	void BuiltInLoader::finalizeTypes() {
		for (nat i = 0; src.types[i].name; i++) {
			const BuiltInType &t = src.types[i];
			if (t.superMode == BuiltInType::superExternal)
				continue;

			Type *type = types[i].borrow();
			findNameSet(t.pkg)->add(type);

			if (t.superMode == BuiltInType::superThread)
				type->setThread(threads[t.super]);
		}
	}

	void BuiltInLoader::checkType(Type *type) {
		if (type->runOn().state == RunOn::any && !type->deepCopyFn()) {
			type->add(steal(CREATE(TypeDeepCopy, type, type)));
		}
	}

	void BuiltInLoader::loadFunctions() {
		for (nat i = 0; src.functions[i].fnPtr; i++) {
			const BuiltInFunction &fn = src.functions[i];

			if (fn.mode & BuiltInFunction::noMember) {
				// This is OK. Free functions should always reside in a package.
				addFn(fn, findPkg(fn.pkg));
			} else if (fn.mode & BuiltInFunction::typeMember) {
				assert((fn.mode & BuiltInFunction::hiddenEngine) == 0, L"hidden engine not supported for members.");
				assert((fn.mode & BuiltInFunction::onThread) == 0, L"thread functions are not supported for members.");
				addFn(fn, types[fn.memberId].borrow());
			} else {
				assert(false, L"Unknown function mode. Either typeMember or noMember should be set!");
				continue;
			}
		}

		// Add functions to classes with hidden bases.
		for (nat i = 0; src.functions[i].fnPtr; i++) {
			const BuiltInFunction &fn = src.functions[i];

			if (fn.mode & BuiltInFunction::typeMember) {
				addFnToHidden(fn, types[fn.memberId].borrow());
			}
		}

		// See if we need to generate any missing functions...
		for (nat i = 0; src.types[i].name; i++) {
			const BuiltInType &t = src.types[i];
			if (t.superMode == BuiltInType::superExternal)
				continue;

			Type *type = types[i].borrow();
			checkType(type);
		}
	}

	Function *BuiltInLoader::createFn(const BuiltInFunction &fn, Type *memberOf) {
		vector<Value> params;

		if (memberOf) {
			// This-parameter!
			params.push_back(Value::thisPtr(memberOf));
		}

		Scope scope(root);
		Value result = findValue(scope, fn.result, e);

		for (nat i = 0; i < fn.params.size(); i++)
			params.push_back(findValue(scope, fn.params[i], e));

		Auto<Function> r;
		if (memberOf != null && fn.name == Type::DTOR) {
			r = nativeDtor(e, memberOf, fn.fnPtr);
		} else if (memberOf) {
			r = nativeMemberFunction(e, memberOf, result, fn.name, params, fn.fnPtr);
		} else if (fn.mode & BuiltInFunction::hiddenEngine) {
			r = nativeEngineFunction(e, result, fn.name, params, fn.fnPtr);
		} else {
			r = nativeFunction(e, result, fn.name, params, fn.fnPtr);
		}

		if (fn.mode & BuiltInFunction::onThread) {
			r->runOn(threads[fn.threadId]);
		}

		if (memberOf != null && (fn.mode & BuiltInFunction::virtualFunction) == 0) {
			r->flags |= namedFinal;
		}


		if (fn.mode & BuiltInFunction::setterFunction)
			r->flags |= namedSetter;

		if (fn.mode & BuiltInFunction::castCtor)
			r->flags |= namedAutoCast;

		return r.ret();
	}

	void BuiltInLoader::addFn(const BuiltInFunction &fn, Type *into) {
		Auto<Function> c = createFn(fn, into);
		into->add(c);
	}

	void BuiltInLoader::addFn(const BuiltInFunction &fn, Package *into) {
		Auto<Function> c = createFn(fn, null);
		into->add(c);
	}

	void BuiltInLoader::addFnToHidden(const BuiltInFunction &fn, Type *into, bool first) {
		if (fn.name == Type::CTOR || fn.name == Type::DTOR)
			return;

		if (!first) {
			// Insert here as well! But check if a similar function already exists first ('overloading' should work).
			Auto<Function> c = createFn(fn, into);
			Auto<SimplePart> name = CREATE(SimplePart, into, c->name, c->params);
			if (!steal(into->find(name)))
				into->add(c);
		}

		CopyMembers::iterator i = copyMembers.find(into);
		if (i != copyMembers.end()) {
			vector<Type *> &t = i->second;
			for (nat i = 0; i < t.size(); i++) {
				addFnToHidden(fn, t[i], false);
			}
		}
	}

	void BuiltInLoader::loadVariables() {
		for (nat i = 0; src.variables[i].name; i++) {
			const BuiltInVar &var = src.variables[i];
			addVar(var, types[var.memberId].borrow());
		}
	}

	void BuiltInLoader::addVar(const BuiltInVar &var, Type *into) {
		Value type = findValue(Scope(root), var.type, e);
		Auto<TypeVarCpp> v = CREATE(TypeVarCpp, e, into, type, var.name, Offset(var.offset));
		into->add(v);

		CopyMembers::iterator i = copyMembers.find(into);
		if (i != copyMembers.end()) {
			vector<Type *> &t = i->second;
			for (nat i = 0; i < t.size(); i++) {
				addVar(var, t[i]);
			}
		}
	}

}
