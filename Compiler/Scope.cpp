#include "stdafx.h"
#include "Scope.h"
#include "Name.h"
#include "Named.h"
#include "Package.h"
#include "Type.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

	Named *find(NameLookup *root, SimpleName *name) {
		if (name->empty())
			return as<Named>(root);

		Named *at = root->find(name->at(0));
		for (nat i = 1; at != null && i < name->count(); i++) {
			at = at->find(name->at(i));
		}

		return at;
	}

	/**
	 * ScopeLookup.
	 */

	ScopeLookup::ScopeLookup() {}

	ScopeLookup::ScopeLookup(Str *v) : voidName(v) {}

	Package *ScopeLookup::firstPkg(NameLookup *l) {
		while (!as<Package>(l))
			l = l->parent();
		return as<Package>(l);
	}

	Package *ScopeLookup::rootPkg(Package *p) {
		NameLookup *at = p;
		while (at->parent())
			at = at->parent();

		assert(as<Package>(at), L"Detached NameLookup found!");
		return as<Package>(at);
	}

	Package *ScopeLookup::corePkg(NameLookup *l) {
		// TODO: Ask the engine directly!
		Package *top = firstPkg(l);
		Package *root = rootPkg(top);
		SimplePart *core = new (l) SimplePart(new (l) Str(L"core"));
		return as<Package>(root->find(core));
	}

	NameLookup *ScopeLookup::nextCandidate(NameLookup *prev) {
		if (Package *pkg = as<Package>(prev)) {
			// Examine the root next.
			Package *root = rootPkg(pkg);
			if (pkg == root)
				return null;
			else
				return root;
		} else {
			// Default behaviour is to look at the nearest parent.
			return prev->parent();
		}
	}

	Named *ScopeLookup::find(Scope in, SimpleName *name) {
		// Regular path.
		for (NameLookup *at = in.top; at; at = nextCandidate(at)) {
			if (Named *found = storm::find(at, name))
				return found;
		}

		// Core.
		if (Package *core = corePkg(in.top))
			if (Named *found = storm::find(core, name))
				return found;

		// We failed!
		return null;
	}

	Value ScopeLookup::value(Scope in, SimpleName *name, SrcPos pos) {
		// TODO: We may want to consider type aliases in the future, and implement 'void' that way.
		if (voidName != null && name->count() == 1) {
			SimplePart *last = name->last();
			if (last->name->equals(voidName) && last->params->empty())
				return Value();
		}

		Type *t = as<Type>(find(in, name));
		if (!t)
			throw SyntaxError(pos, ::toS(name) + L" can not be resolved to a type.");

		return Value(t);
	}


	/**
	 * Scope.
	 */

	Scope::Scope() : top(null), lookup(null) {}

	Scope::Scope(NameLookup *top) : top(top) {
		assert(top);
		lookup = top->engine().scopeLookup();
	}

	Scope::Scope(NameLookup *top, ScopeLookup *lookup) : top(top), lookup(lookup) {}

	Scope::Scope(Scope parent, NameLookup *top) : top(top), lookup(parent.lookup) {}

	void Scope::deepCopy(CloneEnv *env) {
		// Should be OK to not do anything here... All our members are threaded objects.
	}

	Named *Scope::find(Name *name) const {
		SimpleName *simple = name->simplify(*this);
		if (simple)
			return find(simple);

		return null;
	}

	Named *Scope::find(SimpleName *name) const {
		if (lookup)
			return lookup->find(*this, name);

		return null;
	}

	Named *Scope::find(const wchar *name, Array<Value> *params) const {
		return find(new (params) SimpleName(new (params) Str(name), params));
	}

	Value Scope::value(Name *name, SrcPos pos) const {
		if (lookup) {
			SimpleName *simple = name->simplify(*this);
			if (simple)
				return lookup->value(*this, simple, pos);
		}

		throw SyntaxError(pos, ::toS(name) + L" can not be resolved to a type!");
	}

	Value Scope::value(SrcName *name) const {
		return value(name, name->pos);
	}

	wostream &operator <<(wostream &to, const Scope &s) {
		if (!s.lookup) {
			to << L"<empty>";
		} else {
			to << L"<using ";
			to << s.lookup->type()->identifier();
			to << L"> ";

			bool first = true;
			for (NameLookup *at = s.top; at; at = at->parentLookup) {
				if (!first)
					to << " -> ";
				first = false;

				if (Named *n = as<Named>(at)) {
					to << n->identifier();
				} else {
					to << at->type()->identifier();
				}
			}
		}

		return to;
	}

	StrBuf &operator <<(StrBuf &to, Scope s) {
		return to << ::toS(s).c_str();
	}


	/**
	 * ScopeExtra.
	 */

	ScopeExtra::ScopeExtra() {}

	ScopeExtra::ScopeExtra(Str *v) : ScopeLookup(v) {}

	Named *ScopeExtra::find(Scope in, SimpleName *name) {
		if (Named *found = ScopeLookup::find(in, name))
			return found;

		for (nat i = 0; i < extra->count(); i++) {
			if (Named *found = storm::find(extra->at(i), name))
				return found;
		}

		return null;
	}

}
