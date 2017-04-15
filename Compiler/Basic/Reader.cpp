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
			syntax::Rule *r = as<syntax::Rule>(syntaxPkg(this)->find(L"SIncludes"));
			if (!r)
				throw LangDefError(L"Can not find the 'SIncludes' rule.");
			return new (this) syntax::InfoParser(r);
		}

		FileReader *UseReader::createNext(ReaderQuery q) {
			// We do not provide syntax.
			if (q & qSyntax)
				return null;

			syntax::Parser *p = syntax::Parser::create(syntaxPkg(this), L"SIncludes");

			if (!p->parse(info->contents, info->url))
				p->throwError();

			Array<SrcName *> *includes = p->transform<Array<SrcName *>>();
			return new (this) CodeReader(info->next(p->matchEnd()), includes);
		}


		/**
		 * CodeReader.
		 */

		CodeReader::CodeReader(FileInfo *info, Array<SrcName *> *includes) : FileReader(info) {
			BSLookup *lookup = new (this) BSLookup();
			scope = Scope(info->pkg, lookup);

			for (Nat i = 0; i < includes->count(); i++) {
				SrcName *v = includes->at(i);
				Package *p = as<Package>(engine().scope().find(v));
				if (!p)
					throw SyntaxError(v->pos, L"Unknown package " + ::toS(v));

				addInclude(scope, p);
			}
		}

		syntax::InfoParser *CodeReader::createParser() {
			syntax::Rule *r = as<syntax::Rule>(syntaxPkg(this)->find(L"SFile"));
			if (!r)
				throw LangDefError(L"Can not find the 'SFile' rule.");
			syntax::InfoParser *p = new (this) syntax::InfoParser(r);
			addSyntax(scope, p);
			return p;
		}

		void CodeReader::readTypes() {
			readContent();
			Package *pkg = info->pkg;

			for (nat i = 0; i < content->types->count(); i++) {
				pkg->add(content->types->at(i));
			}

			for (nat i = 0; i < content->threads->count(); i++) {
				pkg->add(content->threads->at(i));
			}
		}

		void CodeReader::resolveTypes() {
			readContent();
			Package *pkg = info->pkg;

			for (nat i = 0; i < content->types->count(); i++) {
				if (Class *c = as<Class>(content->types->at(i))) {
					c->lookupTypes();
				}
			}
		}

		void CodeReader::readFunctions() {
			readContent();
			Package *pkg = info->pkg;

			for (nat i = 0; i < content->functions->count(); i++) {
				pkg->add(content->functions->at(i)->createFn());
			}

			for (nat i = 0; i < content->templates->count(); i++) {
				pkg->add(content->templates->at(i));
			}
		}

		void CodeReader::readContent() {
			if (content)
				return;

			syntax::Parser *p = syntax::Parser::create(syntaxPkg(this), L"SFile");
			addSyntax(scope, p);

			p->parse(info->contents, info->url, info->start);
			if (p->hasError())
				p->throwError();

			content = p->transform<Content, Scope>(scope);
		}

	}
}
