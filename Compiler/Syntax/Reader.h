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
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg);

		/**
		 * Reader for files.
		 */
		class FileReader : public storm::FileReader {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FileReader(FileInfo *info);

			// Get the syntax rules.
			virtual void STORM_FN readSyntaxRules();

			// Get the syntax productions.
			virtual void STORM_FN readSyntaxProductions();

		private:
			// Loaded syntax definitions.
			FileContents *c;

			// Scope. Created by 'ensureLoaded'.
			Scope scope;

			// Ensure the contents is loaded.
			void ensureLoaded();
		};


		/**
		 * Custom lookup for the syntax language. Resolves the SStr class properly even if core.lang
		 * is not included.
		 */
		class SyntaxLookup : public ScopeExtra {
			STORM_CLASS;
		public:
			STORM_CTOR SyntaxLookup();

			// Lookup.
			virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);
		};

	}
}
