#include "stdafx.h"
#include "Reader.h"
#include "Class.h"
#include "Core/Io/Text.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Syntax/Parser.h"

namespace storm {
	namespace bs {

		static storm::FileReader *CODECALL createFile(Url *file, Package *pkg) {
			return new (file) FileReader(file, pkg);
		}

		PkgReader *reader(Array<Url *> *files, Package *pkg) {
			Engine &e = pkg->engine();
			return new (e) FilePkgReader(files, pkg, fnPtr(e, &createFile));
		}

		FileReader::FileReader(Url *file, Package *into) : storm::FileReader(file, into) {
			scopeLookup = new (this) BSLookup();
			scope = Scope(into, scopeLookup);
		}

		void FileReader::readTypes() {
			readContent();

			for (nat i = 0; i < content->types->count(); i++) {
				pkg->add(content->types->at(i));
			}

			for (nat i = 0; i < content->threads->count(); i++) {
				pkg->add(content->threads->at(i));
			}
		}

		void FileReader::resolveTypes() {
			readContent();

			for (nat i = 0; i < content->types->count(); i++) {
				if (Class *c = as<Class>(content->types->at(i))) {
					c->lookupTypes();
				}
			}
		}

		void FileReader::readFunctions() {
			readContent();

			for (nat i = 0; i < content->functions->count(); i++) {
				pkg->add(content->functions->at(i)->createFn());
			}

			// TODO: Add templates!
		}

		void FileReader::readContent() {
			if (content)
				return;

			Str *src = readAllText(file);
			Str::Iter i = readIncludes(src);
			readContent(src, i);
		}

		Str::Iter FileReader::readIncludes(Str *src) {
			syntax::Parser *p = syntax::Parser::create(syntaxPkg(), L"SIncludes");
			// TODO: Add syntax from current scope as well?

			if (!p->parse(src, file))
				p->throwError();

			Array<SrcName *> *includes = p->transform<Array<SrcName *>>();
			for (nat i = 0; i < includes->count(); i++) {
				SrcName *v = includes->at(i);
				Package *p = as<Package>(engine().scope().find(v));
				if (!p)
					throw SyntaxError(v->pos, L"Unknown package " + ::toS(v));

				addInclude(scope, p);
			}

			return p->matchEnd();
		}

		void FileReader::readContent(Str *src, Str::Iter start) {
			syntax::Parser *p = syntax::Parser::create(syntaxPkg(), L"SFile");
			addSyntax(scope, p);

			p->parse(src, file, start);
			if (p->hasError())
				p->throwError();

			content = p->transform<Content, Scope>(scope);
		}

		Package *FileReader::syntaxPkg() {
			Type *me = runtime::typeOf(this);
			Package *p = as<Package>(me->parent());
			assert(p);
			return p;
		}

	}
}
