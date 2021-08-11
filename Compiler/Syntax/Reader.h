#pragma once
#include "Core/Array.h"
#include "Core/Io/Url.h"
#include "Compiler/Package.h"
#include "Compiler/Reader.h"
#include "Compiler/FileReader.h"
#include "Compiler/Scope.h"
#include "Decl.h"
#include "Parse.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		// Entry point for the syntax language.
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg) ON(Compiler);

		/**
		 * First stage reader for the syntax language. Here, we only read use statements to figure
		 * out the syntax used in the rest of the file.
		 *
		 * During bootstrapping, this step is skipped entirely.
		 */
		class UseReader : public FileReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR UseReader(FileInfo *info);

			// Create parser.
			virtual syntax::InfoParser *STORM_FN createParser();

		protected:
			// Create the next part.
			virtual MAYBE(FileReader *) STORM_FN createNext(ReaderQuery q);
		};

		class SyntaxLookup;

		/**
		 * Second stage reader for the syntax language. This is where the bulk of the work is done.
		 */
		class DeclReader : public FileReader {
			STORM_CLASS;
		public:
			// Create, assuming we have read some imports previously.
			STORM_CTOR DeclReader(FileInfo *info, Array<SrcName *> *imports);

			// Create, assuming we want to use the C parser.
			STORM_CTOR DeclReader(FileInfo *info);

			// Get the syntax rules.
			virtual void STORM_FN readSyntaxRules();

			// Get the productions.
			virtual void STORM_FN readSyntaxProductions();

			// Create parser.
			virtual syntax::InfoParser *STORM_FN createParser();

		private:
			// Loaded syntax definitions.
			FileContents *c;

			// Scope. Created by 'ensureLoaded'.
			Scope scope;

			// Packages used for extensions.
			Array<SrcName *> *syntax;

			// Ensure contents is loaded.
			void ensureLoaded();

			// Add packages to a ScopeLookup.
			void add(SyntaxLookup *to, Array<SrcName *> *used);

			// Add packages to a parser.
			void add(syntax::ParserBase *to, Array<SrcName *> *used);
		};



		/**
		 * Custom lookup for the syntax language. Resolves the SStr class properly even if core.lang
		 * is not included.
		 */
		class SyntaxLookup : public ScopeExtra {
			STORM_CLASS;
		public:
			STORM_CTOR SyntaxLookup();

			// Clone.
			virtual ScopeLookup *STORM_FN clone() const;

			// Lookup.
			virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);
		};

	}
}
