#include "stdafx.h"
#include "Parser.h"
#include "Package.h"
#include "SStr.h"
#include "Utils/Memory.h"
#include "Core/Runtime.h"
#include "Lib/Parser.h"
#include "Compiler/Lib/Array.h"

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
			Nat r = 0;
			for (nat i = 0; i < steps->count(); i++)
				r += steps->at(i).count();
			return r;
		}

		Nat ParserBase::byteCount() const {
			return sizeof(ParserBase) + steps->count() * sizeof(void *) + stateCount() * sizeof(State);
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
						throw InternalError(::toS(t->identifier()) + L" is not defined correctly.");
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
			// Remember what we parsed.
			src = str;
			srcPos = SrcPos(file, start.offset());
			Nat len = str->peekLength() + 1 - srcPos.pos;
			lastFinish = len + 1;

			// Set up storage.
			steps = new (this) Array<StateSet>();
			steps->reserve(len);
			for (nat i = 0; i < len; i++)
				steps->push(StateSet(engine()));

			// Create the initial production.
			rootProd = new (this) Production();
			rootProd->tokens->push(new (this) RuleToken(root()));

			// Set up the initial state.
			steps->at(0).push(rootProd->firstA(), 0, 0);

			// Process all steps.
			for (nat i = 0; i < len; i++) {
				//PVAR(i);
				if (process(i))
					lastFinish = i;
			}

			return lastFinish < len;
		}

		bool ParserBase::process(nat step) {
			bool seenFinish = false;

			StateSet &src = steps->at(step);
			for (nat i = 0; i < src.count(); i++) {
				State *at = src[i];
				//PVAR(at);

				predictor(step, at);
				completer(step, at);
				scanner(step, at);

				if (at->finishes(rootProd))
					seenFinish = true;
			}

			return seenFinish;
		}

		void ParserBase::predictor(nat step, State *state) {
			RuleToken *rule = state->getRule();
			if (!rule)
				return;

			// Note: this creates new (empty) entries for any rules referred to but not yet included.
			RuleInfo &info = rules->at(rule->rule);

			StateSet &src = steps->at(step);
			for (Nat i = 0; i < info.productions->count(); i++) {
				Production *p = info.productions->at(i);
				src.push(p->firstA(), step, step);
				src.push(p->firstB(), step, step);
			}

			if (matchesEmpty(info)) {
				// The original parser fails with rules like:
				// DELIMITER : " *";
				// Foo : "(" - DELIMITER - Bar - DELIMITER - ")";
				// Bar : DELIMITER;
				// since the completed state of Bar may already have been added and processed when
				// trying to match DELIMITER. Therefore, we need to look for completed instances of
				// Bar (since it may match "") in the current state before continuing. If it is not
				// found, it will be added and processed later, which is OK. This is basically what
				// the completer does (only less general):

				// We do not need to examine further than the current state. Anything else will be
				// examined in the main loop later.
				for (nat i = 0; src[i] != state && i < src.count(); i++) {
					State *now = src[i];
					if (!now->pos.end())
						continue;

					Rule *cRule = now->production()->rule();
					if (cRule != rule->rule)
						continue;

					src.push(state->pos.nextA(), step, state->from, state, now);
					src.push(state->pos.nextB(), step, state->from, state, now);
				}
			}
		}

		void ParserBase::completer(nat step, State *state) {
			if (!state->pos.end())
				return;

			Rule *completed = state->pos.rule();
			StateSet &from = steps->at(state->from);
			for (nat i = 0; i < from.count(); i++) {
				State *s = from[i];
				RuleToken *rule = s->getRule();
				if (!rule)
					continue;
				if (rule->rule != completed)
					continue;

				steps->at(step).push(s->pos.nextA(), step, s->from, s, state);
				steps->at(step).push(s->pos.nextB(), step, s->from, s, state);
			}
		}

		void ParserBase::scanner(nat step, State *state) {
			RegexToken *regex = state->getRegex();
			if (!regex)
				return;

			nat offset = srcPos.pos;
			nat matched = regex->regex.matchRaw(src, step + offset);
			if (matched == Regex::NO_MATCH)
				return;

			// Should not happen, but better safe than sorry!
			if (matched < offset)
				return;
			matched -= offset;
			if (matched > steps->count())
				return;

			steps->at(matched).push(state->pos.nextA(), matched, state->from, state);
			steps->at(matched).push(state->pos.nextB(), matched, state->from, state);
		}

		bool ParserBase::matchesEmpty(Rule *rule) {
			return matchesEmpty(rules->at(rule));
		}

		bool ParserBase::matchesEmpty(RuleInfo &info) {
			if (info.matchesNull < 2)
				return info.matchesNull != 0;

			// Say the rule matches null in case it is recursive!
			// This will provide correct results and prevent endless recursion.
			info.matchesNull = 1;

			bool match = false;
			for (Nat i = 0; i < info.productions->count(); i++) {
				if (matchesEmpty(info.productions->at(i))) {
					match = true;
					break;
				}
			}

			info.matchesNull = match ? 1 : 0;
			return match;
		}

		static const ProductionIter &pick(const ProductionIter &a, const ProductionIter &b) {
			if (!b.valid())
				return a;
			if (!a.valid())
				return b;
			if (a.position() < b.position())
				return b;
			else
				return a;
		}

		bool ParserBase::matchesEmpty(Production *p) {
			// Note: since we try to match against the empty string, we can greedily pick the one of
			// nextA and nextB which is furthest along the production at each step.
			ProductionIter pos = pick(p->firstA(), p->firstB());
			while (pos.valid() && !pos.end()) {
				if (!matchesEmpty(pos.token()))
					return false;

				pos = pick(pos.nextA(), pos.nextB());
			}

			return true;
		}

		bool ParserBase::matchesEmpty(Token *t) {
			if (RegexToken *r = as<RegexToken>(t)) {
				return r->regex.matchRaw(new (this) Str(L"")) != Regex::NO_MATCH;
			} else if (RuleToken *u = as<RuleToken>(t)) {
				return matchesEmpty(u->rule);
			} else {
				assert(false, L"Unknown syntax token type.");
				return false;
			}
		}

		Bool ParserBase::hasError() const {
			if (lastFinish < steps->count() - 1)
				return true;

			return finish() == null;
		}

		Bool ParserBase::hasTree() const {
			return finish() != null;
		}

		Str::Iter ParserBase::matchEnd() const {
			if (finish())
				return src->posIter(srcPos.pos + lastFinish);
			else
				return src->begin();
		}

		Str *ParserBase::errorMsg() const {
			nat pos = lastStep();
			StrBuf *msg = new (this) StrBuf();

			if (pos == steps->count() - 1) {
				*msg << L"Unexpected end of stream.";
			} else {
				Char ch(src->c_str()[pos + srcPos.pos]);
				Str *chStr = new (this) Str(ch);
				*msg << L"Unexpected '" << chStr->escape() << L"'.";
			}

			*msg << L"\nIn progress:";
			Indent z(msg);
			const StateSet &step = steps->at(pos);
			for (nat i = 0; i < step.count(); i++) {
				*msg << L"\n" << step[i]->pos;
			}

			return msg->toS();
		}

		SyntaxError ParserBase::error() const {
			nat pos = lastStep();
			return SyntaxError(srcPos + pos, ::toS(errorMsg()));
		}

		void ParserBase::throwError() const {
			throw error();
		}

		nat ParserBase::lastStep() const {
			for (nat i = steps->count() - 1; i > 0; i--) {
				if (steps->at(i).count() > 0)
					return i;
			}

			// First step is never empty.
			return 0;
		}

		State *ParserBase::finish() const {
			if (lastFinish >= steps->count())
				return null;

			const StateSet &states = steps->at(lastFinish);
			for (nat i = 0; i < states.count(); i++) {
				State *s = states[i];
				if (s->finishes(rootProd))
					return s;
			}

			return null;
		}

		Node *ParserBase::tree() const {
			State *from = finish();
			if (!from)
				throw error();

			// 'from' finishes the dummy 'rootProd', which the user is not interested in. The user
			// is interested in the production that finished 'rootProd', which is located in
			// 'from->completed'.
			assert(from->completed, L"The root state was not completed by anything!");
			return tree(from->completed);
		}

		template <class T>
		static void setValue(Node *node, MemberVar *target, T *elem) {
			int offset = target->offset().current();
			if (isArray(target->type)) {
				// Arrays are initialized earlier.
				OFFSET_IN(node, offset, Array<T *> *)->push(elem);
			} else {
				OFFSET_IN(node, offset, T *) = elem;
			}
		}

		Node *ParserBase::tree(State *from) const {
			Node *result = allocNode(from);
			Production *prod = from->production();

			// Remember capture start and capture end. Set 'repStart' to first token since we will
			// not find it in the loop if the repeat starts at the first token in this production.
			nat repStart = from->from;
			nat repEnd = 0;

			// Traverse the states backwards. The last token in the chain (the one created first) is
			// skipped as that does not contain any information.
			for (State *at = from; at->prev; at = at->prev) {
				State *prev = at->prev;
				Token *token = prev->pos.token();

				if (at->pos.repStart())
					repStart = at->step;
				if (at->pos.repEnd())
					repEnd = at->step;

				// Don't bother if we do not need to store the result anywhere.
				if (!token->target)
					continue;

				Object *match = null;
				if (as<RegexToken>(token)) {
					Str::Iter from = src->posIter(prev->step + srcPos.pos);
					Str::Iter to = src->posIter(at->step + srcPos.pos);

					setValue(result, token->target, new (this) SStr(src->substr(from, to), srcPos + prev->step));
				} else if (as<RuleToken>(token)) {
					assert(at->completed, L"Rule token not completed!");
					setValue(result, token->target, tree(at->completed));
				} else {
					assert(false, L"Unknown token type used for match.");
				}
			}

			// Remember the capture.
			if (prod->repCapture && prod->repCapture->target && repStart <= repEnd) {
				Str::Iter from = src->posIter(repStart);
				Str::Iter to = src->posIter(repEnd);
				setValue(result, prod->repCapture->target, new (this) SStr(src->substr(from, to), srcPos + repStart));
			}

			// Reverse all arrays in this node, as we're adding them backwards.
			reverseNode(result);
			return result;
		}

		Node *ParserBase::allocNode(State *from) const {
			ProductionType *type = from->production()->type();

			// A bit ugly, but this is enough for the object to be considered a proper object when
			// it is populated.
			void *mem = runtime::allocObject(type->size().current(), type);
			Node *r = new (Place(mem)) Node(srcPos + from->from);
			type->vtable->insert(r);

			// Create any arrays needed.
			for (nat i = 0; i < type->arrayMembers->count(); i++) {
				MemberVar *v = type->arrayMembers->at(i);
				int offset = v->offset().current();
				// This will actually create the correct subtype as long as we're creating something
				// inherited from Object (which we are).
				Array<Object *> *arr = new (v->type.type) Array<Object *>();
				OFFSET_IN(r, offset, Object *) = arr;
			}

			return r;
		}

		void ParserBase::reverseNode(Node *node) const {
			ProductionType *t = as<ProductionType>(runtime::typeOf(node));
			if (!t) {
				WARNING(L"Invalid node type found!");
				return;
			}

			for (nat i = 0; i < t->arrayMembers->count(); i++) {
				MemberVar *v = t->arrayMembers->at(i);
				int offset = v->offset().current();

				Array<Object *> *array = OFFSET_IN(node, offset, Array<Object *> *);
				array->reverse();
			}
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