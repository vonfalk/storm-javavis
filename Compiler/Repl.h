#pragma once
#include "Thread.h"

namespace storm {
	STORM_PKG(lang);

	class NameLookup;

	/**
	 * Defines the entry point for a REPL for a language. Any language that supports a REPL may
	 * implement a function 'repl()' which returns an instance of a Repl-class.
	 */
	class Repl : public ObjectOn<Compiler> {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR Repl();

		/**
		 * Result from evaluating something in the REPL.
		 */
		class Result {
			STORM_VALUE;
		public:
			// Success.
			static Result STORM_FN success();
			static Result STORM_FN success(Str *value);

			// Eval resulted in some kind of error.
			static Result STORM_FN error(Str *value);

			// Incomplete input.
			static Result STORM_FN incomplete();

			// Terminate the repl.
			static Result STORM_FN terminate();

			// Does this result represent a success?
			Bool STORM_FN isSuccess() const;

			// Get the result if success.
			MAYBE(Str *) STORM_FN result() const;

			// Does this result represent an error?
			MAYBE(Str *) STORM_FN isError() const;

			// Does this result represent an incomplete input?
			Bool STORM_FN isIncomplete() const;

			// Shall the REPL be terminated?
			Bool STORM_FN isTerminate() const;

		private:
			enum {
				rSuccess,
				rError,
				rIncomplete,
				rTerminate
			};

			Result(int type, Str *value);

			// Type?
			Int type;

			// Data?
			MAYBE(Str *) data;
		};

		// Output any banners this REPL desires. Not called if not executed interactively.
		virtual void STORM_FN greet();

		// Evaluate a line. Return a result with either success or error message, or indicating that
		// more input is required or that the REPL shall terminate. 'context' is a hint about in
		// what context names shall be resolved if possible.
		virtual Result STORM_FN eval(Str *line, MAYBE(NameLookup *) context) ABSTRACT;
	};

}
