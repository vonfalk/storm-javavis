#pragma once
#include "Code/RefSource.h"
#include "Code/Reference.h"
#include "Std.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Code is a block of machine code belonging to a function. This code
	 * takes parameters as described by the Function it belongs to.
	 * The reason Code is not directly in Function is to allow lookup code
	 * segments to perform vtable lookups easily as needed.
	 */
	class Code : public Object {
		STORM_CLASS;
	public:
		// Set up the RefSource. This 'ref' shall be kept updated until the
		// destruction of this object or when 'update' is called with another
		// object.
		void update(code::RefSource &ref);

	protected:
		// A new reference has been found.
		virtual void newRef() = 0;

		// The reference to update.
		code::RefSource *toUpdate;
	};


	/**
	 * Simple subclass for statically allocated chunks of code.
	 */
	class StaticCode : public Code {
		STORM_CLASS;
	public:
		// Code to be executed.
		StaticCode(void *ptr);

	protected:
		// Update ref.
		virtual void newRef();

	private:
		// Code to be executed.
		void *ptr;
	};


	/**
	 * Lazily loaded code.
	 */
	class LazyCode : public Code {
		STORM_CLASS;
	public:
		STORM_CTOR LazyCode() {}

	protected:
		// Update ref.
		virtual void newRef();
	};


	/**
	 * Delegate to another piece of code (no runtime overhead).
	 */
	class DelegatedCode : public Code {
		STORM_CLASS;
	public:
		DelegatedCode(code::Ref ref, const String &title);

	protected:
		// Update ref.
		virtual void newRef();

	private:
		// We refer something else here!
		code::CbReference reference;

		// Updated.
		void updated(void *n);
	};

}
