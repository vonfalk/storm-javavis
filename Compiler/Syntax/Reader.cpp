#include "stdafx.h"
#include "Reader.h"
#include "Engine.h"
#include "Exception.h"
#include "SStr.h"
#include "Rule.h"
#include "Production.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		static storm::FileReader *CODECALL createFile(Url *file, Package *pkg) {
			return new (file) FileReader(file, pkg);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile));
		}


		FileReader::FileReader(Url *file, Package *into) : storm::FileReader(file, into), c(null) {}

		void FileReader::readSyntaxRules() {
			ensureLoaded();

			for (Nat i = 0; i < c->rules->count(); i++) {
				RuleDecl *decl = c->rules->at(i);
				pkg->add(new (this) Rule(decl, scope));
			}
		}

		void FileReader::readSyntaxProductions() {
			ensureLoaded();

			Rule *delimiter = null;
			if (c->delimiter) {
				delimiter = as<Rule>(scope.find(c->delimiter));
				if (!delimiter)
					throw SyntaxError(c->delimiter->pos, L"The delimiter " + ::toS(c->delimiter)
									+ L" does not exist.");
			}

			for (Nat i = 0; i < c->productions->count(); i++) {
				ProductionDecl *decl = c->productions->at(i);
				Str *name = decl->name;
				if (!name)
					name = pkg->anonName();

				pkg->add(new (this) ProductionType(name, decl, delimiter, scope));
			}
		}

		void FileReader::ensureLoaded() {
			if (c)
				return;

			c = parseSyntax(file);

			Scope root = engine().scope();
			SyntaxLookup *lookup = new (this) SyntaxLookup();
			for (nat i = 0; i < c->use->count(); i++) {
				SrcName *name = c->use->at(i);
				Named *found = root.find(name);
				if (!found)
					throw SyntaxError(name->pos, L"The package " + ::toS(name) + L" does not exist!");
				lookup->extra->push(found);
			}

			scope = Scope(pkg, lookup);
		}



		SyntaxLookup::SyntaxLookup() : ScopeExtra(new (engine()) Str(L"void")) {}

		Named *SyntaxLookup::find(Scope in, SimpleName *name) {
			if (name->count() == 1) {
				SimplePart *last = name->last();
				if (wcscmp(last->name->c_str(), L"SStr") == 0 && last->params->empty())
					return SStr::stormType(engine());

				if (last->params->any() && last->params->at(0) != Value()) {
					if (Named *r = last->params->at(0).type->find(last))
						return r;
				}
			}

			return ScopeExtra::find(in, name);
		}

	}
}
