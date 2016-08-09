#pragma once
#include "Named.h"
#include "Template.h"
#include "Core/Array.h"
#include "Core/Map.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A set of named objects, all with the same name but with different parameters.
	 */
	class NameOverloads : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR NameOverloads();

		// Get the number of items in here.
		Nat STORM_FN count() const;

		// Get item #n in here.
		Named *STORM_FN operator [](Nat id) const;
		Named *at(Nat id) const;

		// Add an element.
		void STORM_FN add(Named *item);

		// Add a template.
		void STORM_FN add(Template *item);

		// Generate from a template and adds it to this overloads object.
		// TODO: More care should be taking when dealing with templates and overload resolution!
		//  maybe matches should not be added here, as we can currently not distinguish template-generated
		//  matches from regular ones, which could be bad.
		virtual MAYBE(Named *) STORM_FN createTemplate(SimplePart *from);

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

		// Add a named object.
		virtual void STORM_FN add(Named *item);

		// Add a template.
		virtual void STORM_FN add(Template *item);

		// Get an anonymous name for this NameSet.
		virtual Str *STORM_FN anonName();

		// Get all members.
		virtual Array<Named *> *STORM_FN content();

		// Force loading this NameSet.
		void STORM_FN forceLoad();

		// Find something in here.
		virtual MAYBE(Named *) STORM_FN find(SimplePart *part);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Late initialization.
		virtual void lateInit();

		/**
		 * Lazy-loading callbacks.
		 *
		 * TODO: Move to protected whenever the preprocessor can handle that properly.
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

	protected:
		// Find a named here without bothering with lazy-loading.
		Named *STORM_FN tryFind(SimplePart *part);

	private:
		// Overloads.
		typedef Map<Str *, NameOverloads *> Overloads;
		Map<Str *, NameOverloads *> *overloads;

		// Lazy-loading done?
		bool loaded;

		// Lazy-loading in progress?
		bool loading;

		// Identifier for the next anonymous thing.
		Nat nextAnon;

		// Initialize.
		void init();
	};

}
