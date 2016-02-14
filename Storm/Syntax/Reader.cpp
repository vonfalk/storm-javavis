#include "stdafx.h"
#include "Reader.h"
#include "Parse.h"
#include "Rule.h"
#include "Option.h"
#include "Engine.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		Reader::Reader(Par<PkgFiles> files, Par<Package> pkg) :
			FilesReader(files, pkg) {}

		FileReader *Reader::createFile(Par<Url> url) {
			return CREATE(File, this, url, pkg);
		}

		File::File(Par<Url> file, Par<Package> into) :
			FileReader(file, into) {}

		void File::readSyntaxRules() {
			ensureLoaded();

			for (Nat i = 0; i < contents->rules->count(); i++) {
				Auto<RuleDecl> decl = contents->rules->at(i);
				Auto<Rule> rule = CREATE(Rule, this, decl, scope);
				pkg->add(rule);
			}
		}

		void File::readSyntaxOptions() {
			ensureLoaded();

			for (Nat i = 0; i < contents->options->count(); i++) {
				Auto<OptionDecl> decl = contents->options->at(i);
				Auto<Str> name = decl->name;
				if (!name)
					name = pkg->anonName();

				Auto<Option> option = CREATE(Option, this, name, decl, scope);
				pkg->add(option);
			}
		}

		void File::ensureLoaded() {
			if (contents)
				return;

			contents = parseSyntax(file);
			PVAR(contents);

			Scope *root = engine().scope();

			Auto<ScopeExtra> lookup = CREATE(ScopeExtra, this);
			for (Nat i = 0; i < contents->use->count(); i++) {
				Auto<Name> name = contents->use->at(i);
				Auto<Named> found = root->find(name);
				if (!found)
					throw SyntaxError(SrcPos(file, 0), L"The package " + ::toS(name) + L" does not exist!");

				lookup->add(found);
			}
			scope = Scope(pkg, lookup);
		}

	}
}
