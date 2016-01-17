#include "stdafx.h"
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
		referring(RefManager::refId(copy.refPtr)),
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
		referring = RefManager::refId(source.refPtr);
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

	Ref::Ref() : arena(null), refPtr(null) {}

	Ref::Ref(Arena &arena, nat id) : arena(&arena), refPtr(arena.refManager.lightRefPtr(id)) {}

	Ref::Ref(const RefSource &source) : arena(&source.arena), refPtr(arena->refManager.lightRefPtr(source.getId())) {}

	Ref::Ref(const Reference &reference) : arena(&reference.arena), refPtr(arena->refManager.lightRefPtr(reference.referring)) {}

	Ref::Ref(const Ref &o) : arena(o.arena), refPtr(o.refPtr) {
		RefManager::addLightRef(refPtr);
	}

	Ref::~Ref() {
		if (arena)
			RefManager::removeLightRef(refPtr);
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
		return refPtr == o.refPtr;
	}

	void swap(Ref &a, Ref &b) {
		std::swap(a.arena, b.arena);
		std::swap(a.refPtr, b.refPtr);
	}

	void *Ref::address() const {
		return RefManager::address(refPtr);
	}

	String Ref::targetName() const {
		return RefManager::name(refPtr);
	}

	nat Ref::id() const {
		return RefManager::refId(refPtr);
	}
}
