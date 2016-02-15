#include "stdafx.h"
#include "Parser.h"
#include "ParserTemplate.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		ParserBase::ParserBase() :
			rules(CREATE(MAP_PP(Rule, ArrayP<Option>), this)) {

			ParserType *type = as<ParserType>(myType);
			assert(type, L"An instance of ParserBase was not correctly created. Use Parser instead!");

			// Find the package in which 'type' is located.
			Auto<Rule> root = rootRule();
			Package *pkg = ScopeLookup::firstPkg(root.borrow());
			if (pkg) {
				addSyntax(pkg);
			} else {
				WARNING(L"The rule " << root << L" is not located in any package, not adding one by default.");
			}
		}

		void ParserBase::output(wostream &to) const {
			Auto<Rule> r = rootRule();
			to << L"Parse: " << r->name;
		}

		Rule *ParserBase::rootRule() const {
			// This is verified in the constructor!
			ParserType *type = (ParserType *)myType;
			return type->root();
		}

		void ParserBase::addSyntax(Par<Package> pkg) {
			// We need to make sure the pkg is loaded before we can iterate through it.
			pkg->forceLoad();
			for (NameSet::iterator i = pkg->begin(), end = pkg->end(); i != end; ++i) {
				Named *n = i->borrow();
				if (Rule *rule = as<Rule>(n)) {
					// We only keep track of these to get better error messages!
					// Make sure to create an entry for the rule!
					options(rule);
				} else if (Option *option = as<Option>(n)) {
					Rule *owner = as<Rule>(option->super());
					if (!owner)
						throw InternalError(L"The option " + option->identifier() + L" is not correctly defined!");

					ArrayP<Option> *to = options(owner);
					to->push(capture(option));
				}
			}
		}

		ArrayP<Option> *ParserBase::options(Par<Rule> rule) {
			if (rules->has(rule)) {
				return rules->get(rule).borrow();
			} else {
				Auto<ArrayP<Option>> r = CREATE(ArrayP<Option>, this);
				rules->put(rule, r);
				return r.borrow();
			}
		}

		void ParserBase::addSyntax(Par<ArrayP<Package>> pkg) {
			for (Nat i = 0; i < pkg->count(); i++) {
				addSyntax(pkg->at(i));
			}
		}

		Str::Iter ParserBase::parse(Par<Str> str, SrcPos from) {
			return parse(str, from, str->begin());
		}

		Str::Iter ParserBase::parse(Par<Str> str, SrcPos from, Str::Iter start) {
			// TODO!
			return start;
		}

		Str::Iter ParserBase::parse(Par<Str> str, Par<Url> file) {
			return parse(str, file, str->begin());
		}

		Str::Iter ParserBase::parse(Par<Str> str, Par<Url> file, Str::Iter start) {
			return parse(str, SrcPos(file, 0), start);
		}

		Bool ParserBase::hasError() {
			return false;
		}

		SyntaxError ParserBase::error() const {
			return SyntaxError(SrcPos(), L"TODO!");
		}

		void ParserBase::throwError() const {
			throw error();
		}

		Str *ParserBase::errorMsg() const {
			return CREATE(Str, this, error().what());
		}


	}
}
