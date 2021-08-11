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

		// Find the syntax package for BNF given a type in 'lang.bnf'.
		static Package *syntaxPkg(RootObject *o) {
			Type *t = runtime::typeOf(o);
			Package *p = as<Package>(t->parent());
			assert(p);
			return p;
		}

		static storm::FileReader *CODECALL createFile(FileInfo *info) {
			return new (info) UseReader(info);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile, Compiler::thread(e)));
		}


		/**
		 * UseReader.
		 */

		UseReader::UseReader(FileInfo *info) : FileReader(info) {}

		syntax::InfoParser *UseReader::createParser() {
			syntax::Rule *r = as<syntax::Rule>(syntaxPkg(this)->find(S("SUse"), Scope()));
			if (!r)
				throw new (this) LangDefError(S("Can not find the 'SUse' rule."));
			return new (this) syntax::InfoParser(r);
		}

		FileReader *UseReader::createNext(ReaderQuery q) {
			Package *grammar = syntaxPkg(this);

			// If we're trying to parse our grammar, use the simple C parser. If the language
			// server wants information, use the full-blown solution instead!
			if (grammar == info->pkg && !(q & qParser)) {
				return new (this) DeclReader(info->next(info->contents->begin()));
			}

			syntax::Parser *p = syntax::Parser::create(grammar, S("SUse"));

			if (!p->parse(info->contents, info->url, info->start))
				p->throwError();

			Array<SrcName *> *included = p->transform<Array<SrcName *>>();
			return new (this) DeclReader(info->next(p->matchEnd()), included);
		}


		/**
		 * DeclReader.
		 */

		DeclReader::DeclReader(FileInfo *info, Array<SrcName *> *use) : FileReader(info), c(null), syntax(use) {}

		DeclReader::DeclReader(FileInfo *info) : FileReader(info), c(null), syntax(new (engine()) Array<SrcName *>()) {}

		void DeclReader::readSyntaxRules() {
			ensureLoaded();
			Package *pkg = info->pkg;

			for (Nat i = 0; i < c->rules->count(); i++) {
				RuleDecl *decl = c->rules->at(i);
				pkg->add(new (this) Rule(decl, scope));
			}
		}

		void DeclReader::readSyntaxProductions() {
			ensureLoaded();
			Package *pkg = info->pkg;

			Delimiters *delimiters = c->delimiters(scope);

			for (Nat i = 0; i < c->productions->count(); i++) {
				ProductionDecl *decl = c->productions->at(i);
				Str *name = decl->name;
				if (!name)
					name = pkg->anonName();

				pkg->add(new (this) ProductionType(name, decl, delimiters, scope));
			}
		}

		static void addSyntax(syntax::ParserBase *to, SyntaxLookup *lookup) {
			for (Nat i = 0; i < lookup->extra->count(); i++)
				if (Package *p = as<Package>(lookup->extra->at(i)))
					to->addSyntax(p);
		}

		syntax::InfoParser *DeclReader::createParser() {
			syntax::Rule *r = as<syntax::Rule>(syntaxPkg(this)->find(S("SRoot"), Scope() /* god mode! */));
			if (!r)
				throw new (this) LangDefError(S("Can not find the 'SRoot' rule."));

			// Add additional syntax!
			syntax::InfoParser *p = new (this) syntax::InfoParser(r);
			add(p, syntax);
			return p;
		}

		void DeclReader::ensureLoaded() {
			if (c)
				return;

			Package *grammar = syntaxPkg(this);
			if (grammar == info->pkg) {
				// We're currently parsing lang.bnf. Use the parser written in C...
				c = parseSyntax(info->contents, info->url, info->start);
			} else {
				// Use the 'real' parser!
				Parser *p = Parser::create(grammar, S("SRoot"));

				// Add syntax from the previous step.
				add(p, syntax);

				// Parse!
				p->parse(info->contents, info->url, info->start);
				if (p->hasError())
					p->throwError();
				c = p->transform<FileContents>();
			}

			// Add any newly found use statements.
			SyntaxLookup *lookup = new (this) SyntaxLookup();
			add(lookup, c->use);

			// Add exports.
			addExports(c->exports);

			scope = Scope(info->pkg, lookup);
		}

		void DeclReader::add(SyntaxLookup *to, Array<SrcName *> *use) {
			Scope root = engine().scope();
			for (Nat i = 0; i < use->count(); i++) {
				SrcName *name = use->at(i);
				Named *found = root.find(name);
				if (!found)
					throw new (this) SyntaxError(name->pos,
												TO_S(this, S("The package ") << name << S(" does not exist!")));
				to->extra->push(found);
			}

			Package *bnfPkg = ScopeLookup::firstPkg(runtime::typeOf(this));
			to->extra = expandExports(to->extra, bnfPkg);
		}

		void DeclReader::add(syntax::ParserBase *to, Array<SrcName *> *use) {
			Scope root = engine().scope();
			Array<Package *> *packages = new (this) Array<Package *>();
			packages->reserve(use->count());
			for (Nat i = 0; i < use->count(); i++) {
				SrcName *name = use->at(i);
				Package *found = as<Package>(root.find(name));
				if (!found)
					throw new (this) SyntaxError(name->pos,
												TO_S(this, S("The package ") << name << S(" does not exist!")));

				packages->push(found);
			}

			Package *bnfPkg = ScopeLookup::firstPkg(runtime::typeOf(this));
			to->addSyntax(expandExports(packages, bnfPkg));
		}

		void DeclReader::addExports(Array<SrcName *> *exports) {
				Scope root = engine().scope();
				Package *bnfPkg = ScopeLookup::firstPkg(runtime::typeOf(this));
				for (Nat i = 0; i < exports->count(); i++) {
					if (Package *p = as<Package>(root.find(exports->at(i)))) {
						info->pkg->addExport(p, bnfPkg);
					}
				}
			}


		SyntaxLookup::SyntaxLookup() : ScopeExtra(new (engine()) Str(S("void"))) {}

		ScopeLookup *SyntaxLookup::clone() const {
			SyntaxLookup *copy = new (this) SyntaxLookup();
			copy->extra->append(extra);
			return copy;
		}

		Named *SyntaxLookup::find(Scope in, SimpleName *name) {
			if (name->count() == 1) {
				SimplePart *last = name->last();
				if (*last->name == S("SStr") && last->params->empty())
					return SStr::stormType(engine());

				if (last->params->any() && last->params->at(0) != Value()) {
					if (Named *r = last->params->at(0).type->find(last, in))
						return r;
				}
			}

			return ScopeExtra::find(in, name);
		}

	}
}
