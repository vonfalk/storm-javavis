#pragma once
#include "Reference.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A reference which acts both like a source and a sink. This is useful if one wants to rename a
	 * reference, or create an alias. For example, sometimes it is useful to have two logically
	 * different references to refer to the same underlying function. This is common to make it
	 * possible to introduce vtable lookups when they are needed without paying anything unless they
	 * are needed.
	 */

	class DelegatedRef;

	/**
	 * Create a source which refers to something useful.
	 */
	class DelegatedContent : public Content {
		STORM_CLASS;
	public:
		STORM_CTOR DelegatedContent(Ref referTo);

		// Set to a new reference.
		void STORM_FN set(Ref ref);
		using Content::set;

		// Get the reference we're referring to.
		Ref STORM_FN to() const;
	protected:
		// The reference in charge of updating us.
		DelegatedRef *ref;
	};

	/**
	 * Create a source for when content has been stolen and should refer to something else.
	 */
	class StolenContent : public DelegatedContent {
		STORM_CLASS;
	public:
		STORM_CTOR StolenContent(RefSource *by);

		// Indicate that we're stolen by someone.
		MAYBE(RefSource *) stolenBy() const;
	};

	/**
	 * The reference used by DelegatedContent and DelegatedSrc.
	 */
	class DelegatedRef : public Reference {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR DelegatedRef(DelegatedContent *owner, Ref to);

		virtual void moved(const void *newAddr);

	private:
		DelegatedContent *owner;
	};

}
