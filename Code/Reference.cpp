#include "StdAfx.h"
#include "Reference.h"

#include "Arena.h"

namespace code {

	Reference::Reference(const RefSource &source, const Content &inside) :
		arena(source.arena),
		referring(source.getId()),
		inside(inside) {
		lastAddress = arena.refManager.addReference(this, referring);
	}

	Reference::Reference(const Ref &copy, const Content &inside) :
		arena(*copy.arena),
		referring(copy.referring),
		inside(inside) {
		lastAddress = arena.refManager.addReference(this, referring);
	}

	Reference::~Reference() {
		arena.refManager.removeReference(this, referring);
	}

	void Reference::output(wostream &to) const {
		to << "Reference to " << arena.refManager.name(referring);
	}

	void Reference::set(RefSource &source) {
		arena.refManager.removeReference(this, referring);
		referring = source.getId();
		lastAddress = arena.refManager.addReference(this, referring);
		onAddressChanged(lastAddress);
	}

	void Reference::set(const Ref &source) {
		arena.refManager.removeReference(this, referring);
		referring = source.referring;
		lastAddress = arena.refManager.addReference(this, referring);
		onAddressChanged(lastAddress);
	}


	void Reference::onAddressChanged(void *newAddress) {
		lastAddress = newAddress;
	}

	CbReference::CbReference(RefSource &source, const Content &in) : Reference(source, in) {}
	CbReference::CbReference(const Ref &copy, const Content &in) : Reference(copy, in) {}

	void CbReference::onAddressChanged(void *a) {
		Reference::onAddressChanged(a);
		onChange(a);
	}

	AddrReference::AddrReference(void **update, RefSource &source, const Content &in)
		: Reference(source, in), update(update) {
		*update = source.address();
	}

	AddrReference::AddrReference(void **update, const Ref &from, const Content &in)
		: Reference(from, in), update(update) {
		*update = from.address();
	}

	void AddrReference::onAddressChanged(void *a) {
		Reference::onAddressChanged(a);
		*update = a;
	}

	Ref::Ref() : arena(null), referring(0) {}

	Ref::Ref(Arena &arena, nat id) : arena(&arena), referring(id) {
		addRef();
	}

	Ref::Ref(const RefSource &source) : arena(&source.arena), referring(source.getId()) {
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

	Ref Ref::fromLea(Arena &arena, void *data) {
		return Ref(arena, (nat)data);
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
		return arena->refManager.address(referring);
	}

	String Ref::targetName() const {
		return arena->refManager.name(referring);
	}

	void Ref::addRef() {
		if (arena)
			arena->refManager.addLightRef(referring);
	}


}
