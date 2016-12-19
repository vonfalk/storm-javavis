#include "stdafx.h"
#include "Reader.h"
#include "Core/Io/Text.h"
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
		}

		void FileReader::resolveTypes() {
			readContent();
		}

		void FileReader::readFunctions() {
			readContent();
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

			PLN(L"Parsing " << file);
			if (!p->parse(src, file))
				p->throwError();

			Array<SrcName *> *includes = p->transform<Array<SrcName *>>();
			PVAR(includes);

			return p->matchEnd();
		}

		void FileReader::readContent(Str *src, Str::Iter start) {
			TODO(L"FIXME!");
		}

		Package *FileReader::syntaxPkg() {
			Type *me = runtime::typeOf(this);
			Package *p = as<Package>(me->parent());
			assert(p);
			return p;
		}

	}
}
