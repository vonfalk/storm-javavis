#include "StdAfx.h"
#include "Reference.h"

#include "Arena.h"

namespace code {

	Reference::Reference(RefSource &source, const String &title) : title(title), arena(arena), referring(source.getId()) {
		lastAddress = arena.refManager.addReference(this, referring);
	}

	Reference::Reference(const Ref &copy, const String &title) : title(title), arena(*copy.arena), referring(copy.referring) {
		lastAddress = arena.refManager.addReference(this, referring);
	}

	Reference::~Reference() {
		arena.refManager.removeReference(this, referring);
	}

	void Reference::onAddressChanged(void *newAddress) {
		lastAddress = newAddress;
	}

	Ref::Ref() : arena(null), referring(0) {}

	Ref::Ref(RefSource &source) : arena(&source.arena), referring(source.getId()) {
		addRef();
	}

	Ref::Ref(const Reference &reference) : arena(&reference.arena), referring(reference.referring) {
		addRef();
	}

	Ref::~Ref() {
		if (arena)
			arena->refManager.removeLightRef(referring);
	}

	Ref::Ref(const Ref &o) : arena(o.arena), referring(o.referring) {
		addRef();
	}

	Ref &Ref::operator =(const Ref &o) {
		Ref tmp(o);
		swap(*this, tmp);
		return *this;
	}

	bool Ref::operator ==(const Ref &o) const {
		return arena == o.arena && referring == o.referring;
	}

	void swap(Ref &a, Ref &b) {
		std::swap(a.arena, b.arena);
		std::swap(a.referring, b.referring);
	}

	void *Ref::address() const {
		if (arena)
			return arena->refManager.address(referring);
		else
			return null;
	}

	String Ref::targetName() const {
		if (arena)
			return arena->refManager.name(referring);
		else
			return L"NULL";
	}

	void Ref::addRef() {
		if (arena)
			arena->refManager.addLightRef(referring);
	}


}