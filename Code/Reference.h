#pragma once

#include "RefSource.h"
#include "Utils/InlineSet.h"
#include "Utils/Function.h"

namespace code {

	class Arena;

	// There are two kinds of references: A lightweight one
	// and a more robust one.
	//
	// The lightweight one (Ref) are
	// for use during shorter time intervals, for example
	// while generating code, or in anonymous places such
	// as function pointers. This kind of reference can not
	// be serialized to files, as the system has no way of
	// knowing what is referencing code. On the other hand,
	// they can be treated as simple value types, and they
	// have low overhead in run-time and size. This one can
	// also be moved around freely, as long as the ctor is
	// called as many times as the dtor is called.
	//
	// The other one (Reference) is intended for more permanent
	// storage, such as in the referenced code. This one has support
	// for update notifications when the code has moved. It
	// also supports serialization to file. The downside is
	// that it requires you to use it by pointer and it can
	// therefore not be copied easily.

	class Reference;
	class Reference : NoCopy, public util::SetMember<Reference> {
		friend class Ref;
	public:
		Reference(RefSource &source, const String &title);
		Reference(const Ref &copy, const String &title);
		~Reference();

		// Set to a new source.
		void set(RefSource &source);
		void set(const Ref &copy);

		// Note: onAddressChanged will _not_ be called for the initial address.
		virtual void onAddressChanged(void *newAddress);
		inline void *address() const { return lastAddress; }

	private:
		String title;

		// The Arena we belong to.
		Arena &arena;

		// Which ID are we referring to?
		nat referring;

		// The last seen address.
		void *lastAddress;
	};


	/**
	 * Callback function for a reference.
	 */
	class CbReference : public Reference {
	public:
		CbReference(RefSource &source, const String &title);
		CbReference(const Ref &copy, const String &title);

		Fn<void, void*> onChange;
		virtual void onAddressChanged(void *addr);
	};

	/**
	 * Update a specific address.
	 */
	class AddrReference : public Reference {
	public:
		AddrReference(void **update, RefSource &source, const String &title);
		AddrReference(void **update, const Ref &source, const String &title);

		void **update;
		virtual void onAddressChanged(void *addr);
	};


	class Ref {
		friend class Reference;
		friend void swap(Ref &a, Ref &b);
	public:
		Ref();
		explicit Ref(RefSource &source);
		Ref(const Reference &reference);

		Ref(const Ref &o);
		~Ref();
		Ref &operator =(const Ref &o);

		bool operator ==(const Ref &o) const;
		inline bool operator !=(const Ref &o) const { return !(*this == o); }

		void *address() const;
		// Get the name we're referring to.
		String targetName() const;
	private:
		Arena *arena;
		nat referring;

		void addRef();
	};

}
