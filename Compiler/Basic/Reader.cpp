#include "stdafx.h"
#include "Reader.h"
#include "Class.h"
#include "Core/Io/Text.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Syntax/Parser.h"

namespace storm {
	namespace bs {

		static storm::FileReader *CODECALL createFile(FileInfo *info) {
			return new (info) UseReader(info);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile, Compiler::thread(e)));
		}

		// Find the syntax package for BS given a type in 'lang.bs'.
		static Package *syntaxPkg(RootObject *o) {
			Type *t = runtime::typeOf(o);
			Package *p = as<Package>(t->parent());
			assert(p);
			return p;
		}

		/**
		 * UseReader.
		 */

		UseReader::UseReader(FileInfo *info) : FileReader(info) {}

		syntax::InfoParser *UseReader::createParser() {
			syntax::Rule *r = as<syntax::Rule>(syntaxPkg(this)->find(S("SIncludes"), Scope()));
			if (!r)
				throw new (this) LangDefError(S("Can not find the 'SIncludes' rule."));
			return new (this) syntax::InfoParser(r);
		}

		FileReader *UseReader::createNext(ReaderQuery q) {
			// We do not provide syntax.
			if (q & qSyntax)
				return null;

			syntax::Parser *p = syntax::Parser::create(syntaxPkg(this), S("SIncludes"));

			if (!p->parse(info->contents, info->url, info->start))
				p->throwError();

			Array<SrcName *> *includes = p->transform<Array<SrcName *>>();
			return new (this) CodeReader(info->next(p->matchEnd()), includes, q);
		}


		/**
		 * CodeReader.
		 */

		CodeReader::CodeReader(FileInfo *info, Array<SrcName *> *includes, ReaderQuery query) : FileReader(info) {
			Array<Package *> *inc = new (this) Array<Package *>();
			for (Nat i = 0; i < includes->count(); i++) {
				SrcName *v = includes->at(i);
				Package *p = as<Package>(engine().scope().find(v));
				if (p)
					inc->push(p);
				else if ((query & qParser) == 0)
					// Only complain if we're not parsing interactively.
					throw new (this) SyntaxError(v->pos, TO_S(engine(), S("Unknown package ") << v));
			}

			BSLookup *lookup = new (this) BSLookup(inc);
			scope = Scope(info->pkg, lookup);
		}

		syntax::InfoParser *CodeReader::createParser() {
			syntax::Rule *r = as<syntax::Rule>(syntaxPkg(this)->find(S("SFile"), Scope()));
			if (!r)
				throw new (this) LangDefError(S("Can not find the 'SFile' rule."));
			syntax::InfoParser *p = new (this) syntax::InfoParser(r);
			addSyntax(scope, p);
			return p;
		}

		void CodeReader::readTypes() {
			readContent();
			Package *pkg = info->pkg;

			for (Nat i = 0; i < content->types->count(); i++) {
				pkg->add(content->types->at(i));
			}

			for (Nat i = 0; i < content->threads->count(); i++) {
				pkg->add(content->threads->at(i));
			}
		}

		void CodeReader::resolveTypes() {
			readContent();

			for (Nat i = 0; i < content->types->count(); i++) {
				if (Class *c = as<Class>(content->types->at(i))) {
					c->lookupTypes();
				}
			}
		}

		void CodeReader::readFunctions() {
			readContent();
			Package *pkg = info->pkg;

			for (Nat i = 0; i < content->decls->count(); i++) {
				pkg->add(content->decls->at(i)->create());
			}

			for (Nat i = 0; i < content->templates->count(); i++) {
				pkg->add(content->templates->at(i));
			}
		}

		void CodeReader::resolveFunctions() {
			readContent();

			for (Nat i = 0; i < content->decls->count(); i++) {
				content->decls->at(i)->resolve();
			}
		}

		void CodeReader::readContent() {
			if (content)
				return;

			syntax::Parser *p = syntax::Parser::create(syntaxPkg(this), S("SFile"));
			addSyntax(scope, p);

			p->parse(info->contents, info->url, info->start);
			if (p->hasError())
				p->throwError();

			content = p->transform<Content, Scope>(scope);
		}

	}
}
