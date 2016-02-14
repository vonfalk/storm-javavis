#pragma once
#include "Named.h"
#include "Template.h"

namespace storm {
	STORM_PKG(core.lang);

	class NameSet;

	/**
	 * Represents all Named objects sharing the same name. Used mainly inside NameSet, but is
	 * exposed since it is needed during lookup.
	 */
	class NameOverloads : public ObjectOn<Compiler> {
		STORM_CLASS;

		friend class NameSet;
	public:
		// Create.
		STORM_CTOR NameOverloads();

		// Get the number of items in here.
		Nat STORM_FN count() const;

		// Get item #n in here.
		Named *STORM_FN operator [](Nat id) const;

		// For C++, borrowed ptr.
		Named *at(nat id) const;

		// Add an element.
		void add(Par<Named> item);

		// Create an element from the template (returns null on failure). Does _not_ add it!
		virtual MAYBE(Named) *STORM_FN fromTemplate(Par<NamePart> part);

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// All named items in here.
		vector<Auto<Named>> items;

		// Template inside?
		Auto<Template> templ;
	};

	/**
	 * A set of named objects. Implements an interface that easily retrieves other named members.
	 *
	 * TODO: Should accessing the iterators force a load?
	 * TODO: Remove the iterator and replace it with something fancier?
	 */
	class NameSet : public Named {
		STORM_CLASS;

	private:
		// Overloads.
		typedef hash_map<String, Auto<NameOverloads>> OverloadMap;

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

		// Get the next free anonymous name in this name set.
		virtual Str *STORM_FN anonName();

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

		// Iterators: Note: do not attempt to lazy-load this NameSet!
		iterator begin() const;
		iterator end() const;
		iterator begin(const String &name) const;
		iterator end(const String &name) const;

		// Find all types recursively. TODO: Make this more general.
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
		virtual Named *STORM_FN loadName(Par<NamePart> part);

		// Called in case of #2, load everything. 'loadAll' is supposed to call 'add' for all contents.
		// Returns false if something went wrong and the loading should be re-tried. Throwing an exception
		// is equivalent to returning false in the aspect of lazy-loading.
		virtual Bool STORM_FN loadAll();

		// Force a this NameSet to be loaded if it has not already been done.
		void forceLoad();

		// Find a NamePart (returns borrowed ptr).
		virtual MAYBE(Named) *STORM_FN find(Par<NamePart> part);

		// Compile this NameSet and anything below it.
		virtual void STORM_FN compile();

	protected:
		// Output.
		virtual void output(wostream &to) const;

		// Clear.
		void clear();

		// Find a named here. Does not care about lazy-loading.
		Named *tryFind(Par<NamePart> part);

	private:
		// Overloads.
		OverloadMap overloads;

		// Lazy-loading done?
		bool loaded;

		// Ongoing lazy-loading?
		bool loading;

		// Identifier for the next anonymous thing.
		nat nextAnon;

		// Initialize.
		void init();
	};


}
