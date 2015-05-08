#pragma once
#include "Named.h"
#include "Template.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A set of named objects. Implements an interface that easily retrieves other named members.
	 *
	 * TODO: Should accessing the iterators force a load?
	 * TODO: Remove the iterator and replace it with something fancier?
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

		// Get all members.
		virtual ArrayP<Named> *STORM_FN contents();

		/**
		 * Support for lazy-loading. There are two levels of lazy-loading:
		 * 1: You are asked for a specific Name that does not exist.
		 * 2: You are asked to load the entire package.
		 * This is so that packages can load only sub-packages if everything is not
		 * needed. #2 is expected to be implemented, while #1 can be implemented as
		 * a complement. Whenever #2 has been called, neither #1 nor #2 will be
		 * called afterwards.
		 * TODO: Expose to Storm.
		 */

		// Called in case of #1, find a specific name. The returned value will be inserted.
		virtual Named *loadName(const String &name, const vector<Value> &params);

		// Called in case of #2, load everything. 'loadAll' is supposed to call 'add' for all contents.
		// Returns false if something went wrong and the loading should be re-tried. Throwing an exception
		// is equivalent to returning false in the aspect of lazy-loading.
		virtual bool loadAll();

		// Force a this NameSet to be loaded if it has not already been done.
		void forceLoad();

	protected:
		// Find a NamePart (returns borrowed ptr).
		virtual Named *findHere(const String &name, const vector<Value> &params);

		// Compare two parameter lists. 'our' is stored in here, 'asked' is what is asked for.
		virtual bool candidate(MatchFlags flags, const vector<Value> &our, const vector<Value> &asked) const;

		// Output.
		virtual void output(wostream &to) const;

		// Clear.
		void clear();

		// Find a named here. Does not care about lazy-loading.
		Named *tryFind(const String &name, const vector<Value> &params);

	private:
		// Overloads.
		OverloadMap overloads;

		// Add to a specific overload.
		void add(Overload *to, Par<Named> item);

		// Find from a specific overload.
		Named *tryFind(Overload *from, const vector<Value> &params);

		// Lazy-loading done?
		bool loaded;

		// Ongoing lazy-loading?
		bool loading;

		// Initialize.
		void init();
	};


}
