#pragma once

#include "RefSource.h"
#include "Utils/InlineSet.h"
#include "Utils/Function.h"

namespace code {

	class Arena;

	/**
	 * There are two kinds of references: A lightweight one and a more robust one.
	 *
	 * The lightweight one (Ref) are for use during shorter time intervals, for example while
	 * generating code, or in anonymous places such as function pointers. This kind of reference can
	 * not be serialized to files, as the system has no way of knowing what is referencing code. On
	 * the other hand, they can be treated as simple value types, and they have low overhead in
	 * run-time and size. This one can also be moved around freely, as long as the ctor is called as
	 * many times as the dtor is called.
	 *
	 * The other one (Reference) is intended for more permanent storage, such as in the referenced
	 * code. This one has support for update notifications when the code has moved. It also supports
	 * serialization to file. The downside is that it requires you to use it by pointer and it can
	 * therefore not be copied easily. Reference also allows cycle detection by requiring the user
	 * of it to specify which 'RefSource' it is located inside. This allows the reference system to
	 * detect cycles and resolve these correctly.
	 */

	class Reference;
	class Reference : NoCopy, public util::SetMember<Reference>, public Printable {
		friend class Ref;
	public:
		// The first parameter here is what the reference should refer _to_, while the second
		// indicates what the reference provides. Eg, when a reference updates a piece of code,
		// it is located in that code and should therefore be thought of as _inside_ that
		// RefSource.
		Reference(const RefSource &to, const Content &inside);
		Reference(const Ref &copy, const Content &inside);
		~Reference();

		// Set to a new source.
		void set(RefSource &source);
		void set(const Ref &copy);

		// Note: onAddressChanged will _not_ be called for the initial address.
		virtual void onAddressChanged(void *newAddress);
		inline void *address() const { return lastAddress; }

		// Get the content we're associated with.
		inline const Content &content() const { return inside; }

	protected:
		virtual void output(wostream &to) const;

	private:
		// The Arena we belong to.
		Arena &arena;

		// Which ID are we referring to?
		nat referring;

		// The last seen address.
		void *lastAddress;

		// The content we're inside.
		const Content &inside;
	};


	/**
	 * Callback function for a reference.
	 */
	class CbReference : public Reference {
	public:
		CbReference(RefSource &source, const Content &in);
		CbReference(const Ref &copy, const Content &in);

		Fn<void, void*> onChange;
		virtual void onAddressChanged(void *addr);
	};

	/**
	 * Update a specific address.
	 */
	class AddrReference : public Reference {
	public:
		AddrReference(void **update, RefSource &source, const Content &in);
		AddrReference(void **update, const Ref &source, const Content &in);

		void **update;
		virtual void onAddressChanged(void *addr);
	};


	class Ref {
		friend class Reference;
		friend void swap(Ref &a, Ref &b);
	public:
		Ref();
		Ref(const RefSource &source);
		Ref(const Reference &reference);

		Ref(const Ref &o);
		~Ref();
		Ref &operator =(const Ref &o);

		bool operator ==(const Ref &o) const;
		inline bool operator !=(const Ref &o) const { return !(*this == o); }

		void *address() const;
		// Get the name we're referring to.
		String targetName() const;

		// Get our unique identifier. Mostly intended for backends.
		inline nat id() const { return referring; }

		// Create a Ref from the value we get when doing:
		// lea(ptrX, ref);
		static Ref fromLea(Arena &arena, void *data);

	private:
		Ref(Arena &arena, nat id);

		Arena *arena;

		nat referring;

		void addRef();
	};

}
