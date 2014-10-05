#pragma once
#include "SyntaxToken.h"
#include "SrcPos.h"

namespace storm {

	/**
	 * Describes a syntax rule.
	 */
	class SyntaxRule : public Printable, NoCopy {
		friend class RuleIter;
		friend class SyntaxType;
	public:
		// 'pos' is the start of this rule's definition.
		SyntaxRule(const SrcPos &pos);
		~SyntaxRule();

		// Clear all tokens.
		void clear();

		// What kind of repeat is to be used?
		enum Repeat {
			rNone,
			rZeroOne,
			rZeroPlus,
			rOnePlus,
		};

		// Get the name of the type this rule is a member of. Returns "" if not a member.
		String type() const;
		inline SyntaxType *parent() const { return owner; }

		// Add a token.
		void add(SyntaxToken *token);

		// Set start/end of repeat (only one is supported).
		void startRepeat();
		void endRepeat(Repeat type);

		// Has any repeat?
		inline bool hasRepeat() const { return repType != rNone; }

		// Set function to call on match.
		void setMatchFn(const String &name, const vector<String> &params);

		// This rule's position.
		const SrcPos pos;

	protected:
		virtual void output(std::wostream &to) const;
		void output(std::wostream &to, nat marker) const;

	private:
		// Our owner.
		SyntaxType *owner;

		// List of all tokens to match.
		vector<SyntaxToken*> tokens;

		// Start/end of repeat.
		nat repStart, repEnd;
		Repeat repType;

		// Function name to call.
		String matchFn;
		vector<String> matchFnParams;

		// Helper for repetition outputs.
		void outputRep(std::wostream &to, nat i, nat marker) const;
	};


	/**
	 * Iterator into a rule.
	 */
	class RuleIter : public Printable {
	public:
		// Null-iterator.
		RuleIter();

		// Reference a rule.
		RuleIter(SyntaxRule &rule);

		// Valid iterator?
		bool valid() const;

		// At end?
		bool end() const;

		// Create an iterator that is one step further in the iteration.
		// This will in some cases result in multiple possibilities!
		RuleIter nextA() const;
		RuleIter nextB() const;

		// Get the token at the current location.
		SyntaxToken *token() const;

		// Get the rule we're referencing.
		inline SyntaxRule &rule() const { return *ruleP; }

		// Same point?
		inline bool operator ==(const RuleIter &o) const {
			return ruleP == o.ruleP
				&& tokenId == o.tokenId
				&& repCount == o.repCount;
		}
		inline bool operator !=(const RuleIter &o) const {
			return !(*this == o);
		}

	protected:
		virtual void output(wostream &to) const;
	private:
		// Create and set values.
		RuleIter(SyntaxRule *r, nat token, nat rep);

		// Referenced rule, null if invalid.
		SyntaxRule *ruleP;

		// Which token in the rule?
		nat tokenId;

		// How many repetitions (if any)?
		nat repCount;
	};
}
