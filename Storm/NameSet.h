#pragma once
#include "Named.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A set of named objects. Implements an interface that easily retrieves other named members.
	 */
	class NameSet : public Named {
		STORM_CLASS;

	private:
		// A set of overloads.
		class Overload : NoCopy {
		public:
			// Ctor.
			Overload();

			// Dtor.
			~Overload();

			// Contents.
			vector<Auto<Named> > items;
		};

		// Overloads.
		typedef hash_map<String, Overload*> OverloadMap;

	public:
		// Ctors.
		STORM_CTOR NameSet(Par<Str> name);
		NameSet(const String &name);
		NameSet(const String &name, const vector<Value> &params);

		// Dtor.
		~NameSet();

		// Add a Named.
		void add(Par<Named> item);

		// Find a NamePart (returns borrowed ptr).
		virtual Named *find(Par<NamePart> part) const;
		Named *find(const String &name, const vector<Value> &params) const;

		// Iterator:
		class iterator : public std::iterator<std::bidirectional_iterator_tag, Auto<Named> > {
			friend NameSet;
		public:
			iterator();

			iterator &operator ++();
			iterator operator ++(int);
			iterator &operator --();
			iterator operator --(int);

			bool operator ==(const iterator &o) const;
			bool operator !=(const iterator &o) const;

			Auto<Named> &operator *() const;
			Auto<Named> *operator ->() const;
		private:
			iterator(OverloadMap::const_iterator i, nat pos);

			// State.
			OverloadMap::const_iterator src;
			nat pos;
		};

		// Iterators:
		iterator begin() const;
		iterator end() const;
		iterator begin(const String &name) const;
		iterator end(const String &name) const;

	protected:
		// Compare two parameter lists. 'our' is stored in here, 'asked' is what is asked for.
		virtual bool candidate(const vector<Value> &our, const vector<Value> &asked) const;

		// Output.
		virtual void output(wostream &to) const;

		// Clear.
		void clear();
	private:
		// Overloads.
		OverloadMap overloads;

		// Add to a specific overload.
		void add(Overload *to, Par<Named> item);

		// Find from a specific overload.
		Named *find(Overload *from, const vector<Value> &params) const;
	};


}
