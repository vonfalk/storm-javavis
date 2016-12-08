#include "stdafx.h"
#include "Parser.h"
#include "Package.h"
#include "Core/Runtime.h"
#include "Lib/ParserTemplate.h"

namespace storm {
	namespace syntax {

		ParserBase::ParserBase() {
			rules = new (this) Map<Rule *, RuleInfo>();
			lastFinish = -1;

			assert(as<ParserType>(runtime::typeOf(this)),
				L"ParserBase not properly constructed. Use Parser::create() in C++!");

			// Find the package where 'root' is located and add that!
			if (Package *pkg = ScopeLookup::firstPkg(root())) {
				addSyntax(pkg);
			} else {
				WARNING(L"The rule " << root() << L" is not located in any package. No default package added!");
			}
		}

		void ParserBase::toS(StrBuf *to) const {
			// TODO: use 'toBytes' when present!
			*to << L"Parser for " << root()->identifier() << L", currently using " << stateCount()
				<< L" states = " << byteCount() << L" bytes.";
		}

		Nat ParserBase::stateCount() const {
			return 0;
		}

		Nat ParserBase::byteCount() const {
			return sizeof(ParserBase);
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
					// We only keep track of these to get better error messages!
					// Make sure to create an entry for the rule!
					rules->at(rule);
				} else if (ProductionType *t = as<ProductionType>(n)) {
					Rule *owner = as<Rule>(t->super());
					if (!owner)
						throw InternalError(L"The production " + ::toS(t->identifier()) + L" is not correctly defined.");
					rules->at(owner).push(t->production);
				}
			}
		}

		void ParserBase::addSyntax(Array<Package *> *pkgs) {
			for (nat i = 0; i < pkgs->count(); i++)
				addSyntax(pkgs->at(i));
		}

		Bool ParserBase::parse(Str *str, Url *file) {
			return parse(str, file, str->begin());
		}

		Bool ParserBase::parse(Str *str, Url *file, Str::Iter start) {
			return false;
		}

		/**
		 * Rule info struct.
		 */

		ParserBase::RuleInfo::RuleInfo() : productions(null), matchesNull(2) {}

		void ParserBase::RuleInfo::push(Production *p) {
			if (!productions)
				productions = new (p) Array<Production *>();
			productions->push(p);
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
