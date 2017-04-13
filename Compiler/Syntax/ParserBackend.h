#pragma once
#include "InfoErrors.h"
#include "Compiler/Package.h"
#include "Compiler/Exception.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Syntax/InfoNode.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

#ifdef DEBUG
		extern bool parserDebug;
#define PARSER_PLN(x) if (parserDebug) { PLN(x); }
#define PARSER_PVAR(x) if (parserDebug) { PVAR(x); }
#else
#define PARSER_PLN(x)
#define PARSER_PVAR(x)
#endif


		/**
		 * Represents one of the available parser backends.
		 *
		 * Most of the API is protected and only accessible to the Parser class to ensure
		 * type-safety.
		 */
		class ParserBackend : public ObjectOn<Compiler> {
			STORM_CLASS;
		protected:
			friend class ParserBase;

			STORM_CTOR ParserBackend();

			// Add syntax rules or productions.
			virtual void add(Rule *rule);
			virtual void add(ProductionType *prod);

			// Does this parser contain the same syntax as 'o'?
			virtual Bool sameSyntax(ParserBackend *o);

			// Parse a string. Returns 'true' if we found some match.
			virtual Bool parse(Rule *root, Str *str, Url *file, Str::Iter start);

			// Parse a string, doing error recovery. Only call 'infoTree' after parsing in this
			// manner, as the resulting syntax tree is not neccessarily complete.
			virtual Bool parseApprox(Rule *root, Str *str, Url *file, Str::Iter start);

			// Clear all parse-related information. Included packages are retained.
			virtual void clear();

			/**
			 * Operations on the last parse.
			 */

			// Found any errors? If Str::Iter is not end, this is always true. Note that even if we
			// have an error, it could be possible to extract a tree!
			virtual Bool hasError() const;

			// Is it possible to extract a syntax tree? (equivalent to the return value of 'parse').
			virtual Bool hasTree() const;

			// Return an iterator after the last matched character, or the start of the string if no
			// match could be made.
			virtual Str::Iter matchEnd() const;

			// Get the error message.
			virtual Str *errorMsg() const;

			// Get the error position.
			virtual SrcPos errorPos() const;

			// Get the syntax tree. Only for C++, in Storm we know the exact subtype we will generate!
			virtual Node *tree() const;

			// Get the generic syntax tree.
			virtual InfoNode *infoTree() const;

			/**
			 * Performance inspection:
			 */

			// Get the number of states used.
			virtual Nat stateCount() const;

			// Get the number of bytes used.
			virtual Nat byteCount() const;
		};

	}
}
