#pragma once
#include "Core/Thread.h"
#include "Core/Fn.h"
#include "Code/Reference.h"
#include "Code/DelegatedRef.h"
#include "Code/Listing.h"
#include "Code/Binary.h"
#include "Value.h"
#include "CodeGen.h"
#include "InlineParams.h"

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
		code::DelegatedContent *content;
	};


	/**
	 * Subclass for statically allocated chunks of code. Automatically insert a 'EnginePtr' as the
	 * first parameter to the function.
	 */
	class StaticEngineCode : public Code {
		STORM_CLASS;
	public:
		StaticEngineCode(const void *code);

	protected:
		virtual void STORM_FN newRef();

	private:
		// Code to be executed.
		code::Binary *code;

		// Reference to the original code.
		code::RefSource *original;

		// Generate code which does the redirection. Assumes 'owner' is set.
		code::Listing *redirectCode(code::Ref ref);
	};


	/**
	 * Lazily generated code.
	 */
	class LazyCode : public Code {
		STORM_CLASS;
	public:
		// The 'generate' function will be called when code needs to be generated.
		STORM_CTOR LazyCode(Fn<CodeGen *> *generate);

		// Compile.
		virtual void STORM_FN compile();

		// Called to update code.
		static const void *CODECALL updateCode(LazyCode *c);

	protected:
		// Update reference.
		virtual void STORM_FN newRef();

	private:
		// Currently used code.
		code::Binary *binary;

		// Generate code using this function.
		Fn<CodeGen *> *generate;

		// Code loaded?
		bool loaded;

		// Code loading?
		bool loading;

		// Called to update code from the Compiler thread.
		static const void *CODECALL updateCodeLocal(LazyCode *c);

		// Create a redirect chunk of code.
		void createRedirect();

		// Set the code in here.
		void setCode(code::Listing *src);
	};


	/**
	 * Subclass for inlined code. Lazily provides a non-inlined variant as well.
	 */
	class InlineCode : public LazyCode {
		STORM_CLASS;
	public:
		STORM_CTOR InlineCode(Fn<void, InlineParams> *create);

		// Generate inlined code.
		virtual void STORM_FN code(CodeGen *state, Array<code::Operand> *params, CodeResult *result);

	private:
		// Generate function.
		Fn<void, InlineParams> *create;

		// Generate a non-inline version as well.
		CodeGen *CODECALL generatePtr();
	};

}
