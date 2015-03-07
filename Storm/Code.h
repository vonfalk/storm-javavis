#pragma once
#include "Code/RefSource.h"
#include "Code/Reference.h"
#include "Utils/Semaphore.h"
#include "Std.h"
#include "CodeGen.h"

namespace storm {
	STORM_PKG(core.lang);

	class Function;

	/**
	 * Code is a block of machine code belonging to a function. This code
	 * takes parameters as described by the Function it belongs to.
	 * The reason Code is not directly in Function is to allow lookup code
	 * segments to perform vtable lookups easily as needed.
	 */
	class Code : public Object {
		STORM_CLASS;
	public:
		Code();

		// Attach to a specific function. Done by the function itself
		// before 'update' is called.
		void attach(Function *owner);

		// Detach the old owner.
		void detach();

		// Set up the RefSource. This 'ref' shall be kept updated until the
		// destruction of this object or when 'update' is called with another
		// object.
		void update(code::RefSource &ref);

	protected:
		// A new reference has been found.
		virtual void newRef() = 0;

		// The reference to update.
		code::RefSource *toUpdate;

		// Owning function.
		Function *owner;
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
	 * Simple subclass for dynamic generated code.
	 */
	class DynamicCode : public Code {
		STORM_CLASS;
	public:
		// Code to be executed.
		DynamicCode(const code::Listing &listing);

		~DynamicCode();

	protected:
		// Update ref.
		virtual void newRef();

	private:
		// Binary with code.
		code::Binary *code;
	};


	/**
	 * Lazily loaded code.
	 */
	class LazyCode : public Code {
		STORM_CLASS;
	public:
		// The 'generate' function will be called when code needs to be updated.
		LazyCode(const Fn<code::Listing, void> &generate);

		// Dtor.
		~LazyCode();

	protected:
		// Update ref.
		virtual void newRef();

	private:
		// Currently used code.
		code::Binary *code;

		// Generate code using this function.
		Fn<code::Listing, void> load;

		// Code loaded?
		bool loaded;

		// Loading code?
		bool loading;

		// Create redirection code.
		void createRedirect();

		// Called to update code.
		static const void *CODECALL updateCode(LazyCode *c);

		// Message between the two versions of updateCode.
		struct Msg {
			// Note: the use of the 'wrong' semaphore here is intentional. Otherwise we would
			// break multithreading semantics by allowing thread switches at every function call
			// the first time that function is called.
			Semaphore sema;
			const void *result;
			inline Msg() : sema(0), result(null) {}
		};

		// Update code, know we're on the right thread.
		static void updateCodeLocal(LazyCode *c, Msg *msg);

		// Create a Binary from a Listing.
		void setCode(const code::Listing &listing);
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


	/**
	 * Parameters to a function generating inlined code.
	 */
	struct InlinedParams {
		Engine &engine;
		const GenState &state;
		const vector<code::Value> &params;
		GenResult &result;
	};


	/**
	 * Inlined code. Lazily provides actual code as well.
	 */
	class InlinedCode : public LazyCode {
		STORM_CLASS;
	public:
		InlinedCode(Fn<void, InlinedParams> generate);

		// Generate inlined code.
		virtual void code(const GenState &state, const vector<code::Value> &params, GenResult &result);
	private:
		// Generation fn.
		Fn<void, InlinedParams> generate;

		// Generate non-inline version as well.
		code::Listing generatePtr();
	};

}
