#pragma once
#include "Named.h"
#include "Template.h"
#include "Core/Array.h"
#include "Core/Map.h"

namespace storm {
	STORM_PKG(core.lang);

	class NameSet;

	/**
	 * Interface receiving differences between two NameSets.
	 *
	 * Not intended to be used from Storm; only intended for reload internals.
	 */
	class NameDiff {
	public:
		// Called for each entity that was added.
		virtual void added(Named *item) = 0;
		virtual void added(Template *item) = 0;

		// Called for each entity that was removed.
		virtual void removed(Named *item) = 0;
		virtual void removed(Template *item) = 0;

		// Called for each entity that exists in both versions. Note: We can't diff templates, so
		// they are never treated as being 'equal'.
		virtual void changed(Named *old, Named *changed) = 0;
	};

	/**
	 * A set of named objects, all with the same name but with different parameters.
	 */
	class NameOverloads : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR NameOverloads();

		// Is this instance empty? (ie. not even any template?)
		Bool STORM_FN empty() const;

		// Get the number of items in here. Note: May return 0 even if 'empty' returns false.
		Nat STORM_FN count() const;

		// Get item #n in here.
		Named *STORM_FN operator [](Nat id) const;
		Named *at(Nat id) const;

		// Check if an element is in here.
		Bool STORM_FN has(Named *item);

		// Add an element.
		void STORM_FN add(Named *item);

		// Add a template.
		void STORM_FN add(Template *item);

		// Remove an element. Returns true on success.
		Bool STORM_FN remove(Named *item);

		// Remove a template. Returns true on success.
		Bool STORM_FN remove(Template *item);

		// Merge from another NameOverloads.
		void STORM_FN merge(NameOverloads *from);

		// Diff in various situations.
		void diff(NameOverloads *with, NameDiff &callback);
		void diffAdded(NameDiff &callback);
		void diffRemoved(NameDiff &callback);
		void diffTemplatesAdded(NameDiff &callback);
		void diffTemplatesRemoved(NameDiff &callback);

		// Generate from a template and adds it to this overloads object.
		// TODO: More care should be taking when dealing with templates and overload resolution!
		//  maybe matches should not be added here, as we can currently not distinguish template-generated
		//  matches from regular ones, which could be bad.
		virtual MAYBE(Named *) STORM_FN createTemplate(NameSet *owner, SimplePart *from);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// All named items.
		Array<Named *> *items;

		// Templates with this name.
		Array<Template *> *templates;
	};

	/**
	 * A named object containing other named objects. Used for building recursive trees inside the
	 * compiler.
	 *
	 * Implements support for lazy-loading. At creation, the NameSet assumes there is some amount of
	 * content that can be loaded on demand. When someone tries to access the content in the
	 * NameSet, it tries to load as little as possible while still determining if the content exists.
	 *
	 * Lazy-loading happens in two steps:
	 * 1: The NameSet asks the derived class to load a specific Name that does not yet exist.
	 * 2: The NameSet asks the derived class to load all content.
	 *
	 * The first step does not need to be implemented, while the second step is mandatory. As soon
	 * as #2 has been called, the NameSet assumes that all content is loaded, and will therefore
	 * never call #1 again. Pay attention if you implement both, that #2 may not load things that #1
	 * have previously loaded.
	 *
	 * Content can of course be added eagerly by calling 'add'. This does not affect the
	 * lazy-loading process in any way.
	 */
	class NameSet : public Named {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR NameSet(Str *name);
		STORM_CTOR NameSet(Str *name, Array<Value> *params);

		// Check if this name set has a Named with the exact parameters as the provided
		// entity. I.e., would it be possible to store 'item' here without issues?
		virtual Bool STORM_FN has(Named *item) const;

		// Add a named object.
		virtual void STORM_FN add(Named *item);

		// Add a template.
		virtual void STORM_FN add(Template *item);

		// Remove a named object from here.
		virtual Bool STORM_FN remove(Named *item);

		// Remove a template from here.
		virtual Bool STORM_FN remove(Template *item);

		// Get an anonymous name for this NameSet.
		virtual Str *STORM_FN anonName();

		// Get all members.
		virtual Array<Named *> *STORM_FN content();

		// Force loading this NameSet.
		void STORM_FN forceLoad();

		// Find something in here.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part, Scope source);
		using Named::find;

		// Watch this NameSet for new additions.
		virtual void STORM_FN watchAdd(Named *notifyTo);

		// Remove a previously added watch.
		virtual void STORM_FN watchRemove(Named *notifyTo);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Late initialization.
		virtual void lateInit();

		// Force compilation.
		virtual void STORM_FN compile();

		// Discard source information.
		virtual void STORM_FN discardSource();

		// Iterator. TODO: How to do wrt threading?
		class Iter {
			STORM_VALUE;
			friend class NameSet;
		public:
			// Create an iterator pointing to the end.
			STORM_CTOR Iter();

			// Compare.
			Bool STORM_FN operator ==(const Iter &o) const;
			Bool STORM_FN operator !=(const Iter &o) const;

			// Get the value.
			Named *STORM_FN v() const;

			// Increment.
			Iter &STORM_FN operator ++();
			Iter STORM_FN operator ++(int d);

		private:
			// Create an iterator to the start.
			Iter(Map<Str *, NameOverloads *> *c, NameSet *next);

			// Current position in the map.
			typedef Map<Str *, NameOverloads *>::Iter MapIter;
			UNKNOWN(MapBase::Iter) MapIter name;

			// Next NameSet to visit (if any).
			MAYBE(NameSet *) nextSet;

			// Current position in NameOverloads at 'pos'.
			Nat pos;

			// Advance 'name' until we find something!
			void advance();
		};

		// Get iterators to the begin and end of the contents.
		virtual Iter STORM_FN begin() const;
		virtual Iter STORM_FN end() const;

		// Get all overloads for a specific name.
		Array<Named *> *STORM_FN findName(Str *name) const;

		// Dump the internal contents of the NameSet for debugging.
		void dbg_dump() const;

	protected:
		/**
		 * Lazy-loading callbacks.
		 */

		// Load a single missing name (#1 above). Assumed to call add() on one or more candidates
		// for 'part', and then return true. It is acceptable to add no candidates and return true,
		// which is interpreted as 'no matches can be lazy-loaded'. If there may be candidates, but
		// none can be loaded from here, return 'false' and 'loadAll' will be executed.
		virtual Bool STORM_FN loadName(SimplePart *part);

		// Called when we need to load all content (#2 above). Assumed to call add() on all
		// content. Returns false or throws an exception on failure. Failure will re-try loading at
		// a later time.
		virtual Bool STORM_FN loadAll();

		// See if this NameSet is loaded fully.
		inline Bool STORM_FN allLoaded() const { return loaded; }

		// Find a named here without bothering with lazy-loading.
		MAYBE(Named *) STORM_FN tryFind(SimplePart *part, Scope source);

		// Get an iterator that visits another name set after the current one.
		Iter STORM_FN begin(NameSet *after) const;

		// Import entities from another NameSet.
		void STORM_FN merge(NameSet *from);

		// Find differences between two NameSets. For terminology, assumes that 'with' is the "new" version.
		void diff(NameSet *with, NameDiff &callback);

	private:
		// Overloads.
		typedef Map<Str *, NameOverloads *> Overloads;
		Map<Str *, NameOverloads *> *overloads;

		// Lazy-loading done?
		Bool loaded;

		// Lazy-loading in progress?
		Bool loading;

		// Identifier for the next anonymous thing.
		Nat nextAnon;

		// Initialize.
		void init();

		// All Named object who want notifications from us. May be null.
		WeakSet<Named> *notify;

		// Notify something has been added.
		void notifyAdd(Named *what);

		// Notify something has been removed.
		void notifyRemove(Named *what);
	};

}
