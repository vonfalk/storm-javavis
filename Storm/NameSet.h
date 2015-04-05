#pragma once
#include "Named.h"
#include "Template.h"

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

			// Template inside?
			// TODO: Allow multiple with the same name?
			Auto<Template> templ;
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
		virtual void STORM_FN add(Par<Named> item);

		// Add a template.
		virtual void STORM_FN add(Par<Template> item);

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
			iterator(const OverloadMap &m, OverloadMap::const_iterator i, nat pos);

			// State.
			const OverloadMap *m;
			OverloadMap::const_iterator src;
			nat pos;
		};

		// Iterators:
		iterator begin() const;
		iterator end() const;
		iterator begin(const String &name) const;
		iterator end(const String &name) const;

		// Find all types recursively.
		vector<Auto<Type>> findTypes() const;
		void findTypes(vector<Auto<Type>> &t) const;

	protected:
		// Find a NamePart (returns borrowed ptr).
		virtual Named *findHere(const String &name, const vector<Value> &params);

		// Compare two parameter lists. 'our' is stored in here, 'asked' is what is asked for.
		virtual bool candidate(MatchFlags flags, const vector<Value> &our, const vector<Value> &asked) const;

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
		Named *findHere(Overload *from, const vector<Value> &params);
	};


}
