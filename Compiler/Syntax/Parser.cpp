#include "stdafx.h"
#include "Parser.h"
#include "Package.h"
#include "Earley/Parser.h"
#include "Lib/Parser.h"

// Remove these?
// #include "SStr.h"
// #include "Utils/Memory.h"
// #include "Core/Runtime.h"
// #include "Compiler/Lib/Array.h"

namespace storm {
	namespace syntax {

#ifdef DEBUG
		bool parserDebug = false;
#endif

		ParserBase::ParserBase() {
			assert(as<ParserType>(runtime::typeOf(this)),
				L"ParserBase not properly constructed. Use Parser::create() in C++!");

			init(root());
		}

		ParserBase::ParserBase(Rule *root) {
			init(root);
		}

		void ParserBase::init(Rule *root) {
			// TODO: Make it possible to specify which parser to use on creation!
			use = new (this) earley::Parser();

			// Find the package where 'root' is located and add that!
			if (Package *pkg = ScopeLookup::firstPkg(root)) {
				addSyntax(pkg);
			} else {
				WARNING(L"The rule " << root << L" is not located in any package. No default package added!");
			}
		}

		void ParserBase::toS(StrBuf *to) const {
			// TODO: use 'toBytes' when present!
			*to << L"Parser for " << root()->identifier() << L", currently using " << stateCount()
				<< L" states = " << byteCount() << L" bytes.";
		}

		Nat ParserBase::stateCount() const {
			return use->stateCount();
		}

		Nat ParserBase::byteCount() const {
			return use->byteCount();
		}

		Rule *ParserBase::root() const {
			// This is verified in the constructor.
			ParserType *type = (ParserType *)runtime::typeOf(this);
			return type->root;
		}

		void ParserBase::addSyntax(Package *pkg) {
			pkg->forceLoad();
			for (NameSet::Iter i = pkg->begin(), e = pkg->end(); i != e; ++i) {
				Named *n = i.v();
				if (Rule *rule = as<Rule>(n)) {
					use->add(rule);
				} else if (ProductionType *t = as<ProductionType>(n)) {
					use->add(t);
				}
			}
		}

		void ParserBase::addSyntax(Array<Package *> *pkgs) {
			for (nat i = 0; i < pkgs->count(); i++)
				addSyntax(pkgs->at(i));
		}

		Bool ParserBase::sameSyntax(ParserBase *o) {
			return use->sameSyntax(o->use);
		}

		void ParserBase::clear() {
			use->clear();
		}

		Bool ParserBase::parse(Str *str, Url *file) {
			return parse(str, file, str->begin());
		}

		Bool ParserBase::parse(Str *str, Url *file, Str::Iter start) {
			return use->parse(root(), str, file, start);
		}

		Bool ParserBase::hasError() const {
			return use->hasError();
		}

		Bool ParserBase::hasTree() const {
			return use->hasTree();
		}

		Str::Iter ParserBase::matchEnd() const {
			return use->matchEnd();
		}

		Str *ParserBase::errorMsg() const {
			return use->errorMsg();
		}

		SyntaxError ParserBase::error() const {
			return SyntaxError(use->errorPos(), ::toS(errorMsg()));
		}

		void ParserBase::throwError() const {
			throw error();
		}

		Node *ParserBase::tree() const {
			if (Node *r = use->tree())
				return r;
			throw error();
		}

		InfoNode *ParserBase::infoTree() const {
			if (InfoNode *r = use->infoTree())
				return r;
			throw error();
		}


		/**
		 * Info parser.
		 */

		InfoParser::InfoParser(Rule *root) : ParserBase(root), rootRule(root) {}

		void InfoParser::root(Rule *r) {
			rootRule = r;
		}

		Rule *InfoParser::root() const {
			return rootRule;
		}


		/**
		 * C++ interface.
		 */

		Parser::Parser() {}

		Parser *Parser::create(Rule *root) {
			// Pick an appropriate type for it (!= the C++ type).
			Type *t = parserType(root);
			void *mem = runtime::allocObject(sizeof(Parser), t);
			Parser *r = new (Place(mem)) Parser();
			t->vtable->insert(r);
			return r;
		}

		Parser *Parser::create(Package *pkg, const wchar *name) {
			if (Rule *r = as<Rule>(pkg->find(name))) {
				return create(r);
			} else {
				throw InternalError(L"Can not find the rule " + ::toS(name) + L" in " + ::toS(pkg->identifier()));
			}
		}

	}
}
