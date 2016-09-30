#pragma once
#include "Core/Thread.h"
#include "Code/Reference.h"
#include "Code/Listing.h"
#include "Code/Binary.h"

namespace storm {
	STORM_PKG(core.lang);

	class Function;

	/**
	 * Code is a block of machine code belonging to a function. This code takes parameters as
	 * described by the Function it belongs to. The reason we're not using code::Binary directly is
	 * to easier management of lazy compilation, which code::Binary does not support in an easy way.
	 */
	class Code : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Code();

		// Attach to a specific function. DOne by the function itself before 'update' is called.
		void STORM_FN attach(Function *owner);

		// Detach from the old owner.
		void STORM_FN detach();

		// Set up the RefSource. This 'ref' shall be kept updated until the destruction of this
		// object or when 'update' is called with another object.
		void STORM_FN update(code::RefSource *ref);

		// Force compilation (if needed).
		virtual void STORM_FN compile();

	protected:
		// A new reference has been found.
		virtual void STORM_FN newRef();

		// The reference to update.
		MAYBE(code::RefSource *) toUpdate;

		// Owning function.
		MAYBE(Function *) owner;
	};


	/**
	 * Statically allocated chunks of code.
	 */
	class StaticCode : public Code {
		STORM_CLASS;
	public:
		// Code to be executed.
		StaticCode(const void *ptr);

	protected:
		virtual void STORM_FN newRef();

	private:
		const void *ptr;
	};


	/**
	 * Dynamically generated code (eagerly compiled).
	 */
	class DynamicCode : public Code {
		STORM_CLASS;
	public:
		STORM_CTOR DynamicCode(code::Listing *code);

	protected:
		virtual void STORM_FN newRef();

	private:
		code::Binary *binary;
	};


	/**
	 * Delegate to another piece of code (no runtime overhead).
	 */
	class DelegatedCode : public Code {
		STORM_CLASS;
	public:
		STORM_CTOR DelegatedCode(code::Ref ref);

		// Called when the code has been moved.
		void moved(const void *to);

	protected:
		virtual void STORM_FN newRef();

	private:
		// Our content.
		code::Content *content;

		// Our reference to the actual code.
		code::Reference *ref;
	};


	/**
	 * Reference for 'DelegatedCode'.
	 */
	class DelegatedRef : public code::Reference {
		STORM_CLASS;
	public:
		STORM_CTOR DelegatedRef(DelegatedCode *code, code::Ref ref, code::Content *from);

		virtual void moved(const void *newAddr);

	private:
		DelegatedCode *owner;
	};

}
