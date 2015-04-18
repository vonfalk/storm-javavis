#pragma once
#include "SyntaxToken.h"
#include "SrcPos.h"
#include "Scope.h"

namespace storm {

	/**
	 * Describes a syntax rule.
	 */
	class SyntaxOption : public Printable, NoCopy {
		friend class OptionIter;
		friend class SyntaxRule;
	public:
		// 'pos' is the start of this rule's definition.
		SyntaxOption(const SrcPos &pos, const Scope &scope, const String &owner);
		~SyntaxOption();

		// Clear all tokens.
		void clear();

		// What mark is used?
		enum Mark {
			mNone,
			// Repeat values
			mRepZeroOne,
			mRepZeroPlus,
			mRepOnePlus,
			// Capture
			mCapture,
		};

		// Get the name of the rule this option is a member of. Returns "" if not a member.
		inline const String &rule() const { return owner; }

		// Add a token.
		void add(SyntaxToken *token);

		// Set start/end of a mark (only one is supported).
		void startMark();
		void endMark(Mark type);
		void endMark(const String &captureTo);

		// Mark used?
		inline bool hasMark() const { return markType != mNone; }

		// Capture used?
		inline bool hasCapture() const { return markType == mCapture; }
		// To what variable?
		inline const String &captureTo() const { return markCapture; }

		// Syntactic scope.
		const Scope scope;

		// Function name to call/variable's value.
		String matchFn;
		vector<String> matchFnParams;
		bool matchVar;

		// This rule's position.
		const SrcPos pos;

		// This option's priority.
		int priority;

	protected:
		virtual void output(std::wostream &to) const;
		void output(std::wostream &to, nat marker) const;

	private:
		// Our owner.
		String owner;

		// List of all tokens to match.
		vector<SyntaxToken*> tokens;

		// Start/end of repeat.
		nat markStart, markEnd;
		Mark markType;
		String markCapture;

		// Helper for repetition outputs. (0 = nothing, 1 = start, 2 = end)
		int outputMark(std::wostream &to, nat i, nat marker) const;

	};


	/**
	 * Iterator into a rule.
	 */
	class OptionIter : public Printable {
	public:
		// Null-iterator.
		OptionIter();

		// Start of an option:
		static OptionIter firstA(SyntaxOption &option);
		static OptionIter firstB(SyntaxOption &option);

		// Valid iterator?
		bool valid() const;

		// At end?
		bool end() const;

		// At beginning/end of capture:
		// At the first included token in the capture?
		bool captureBegin() const;
		// At the first non-included token in the capture?
		bool captureEnd() const;

		// Create an iterator that is one step further in the iteration.
		// This will in some cases result in multiple possibilities!
		OptionIter nextA() const;
		OptionIter nextB() const;

		// Get the token at the current location.
		SyntaxToken *token() const;

		// Get the option we're referencing.
		inline SyntaxOption &option() const { return *optionP; }

		// Same point?
		inline bool operator ==(const OptionIter &o) const {
			return optionP == o.optionP
				&& tokenId == o.tokenId;
			/* && repCount == o.repCount; Prevents infinite repetitions of zero-rules, but lies a little. */
		}
		inline bool operator !=(const OptionIter &o) const {
			return !(*this == o);
		}

	protected:
		virtual void output(wostream &to) const;
	private:
		// Create and set values.
		OptionIter(SyntaxOption *r, nat token, nat rep);

		// Referenced rule, null if invalid.
		SyntaxOption *optionP;

		// Which token in the rule?
		nat tokenId;

		// How many repetitions (if any)?
		nat repCount;
	};
}
