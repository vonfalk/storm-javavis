#pragma once
#include "SyntaxVariable.h"
#include "SyntaxOption.h"

namespace storm {

	/**
	 * A syntax node. This is the result of parsing a string according to
	 * some grammar. Each node contains all captured variables from the
	 * rule in question. Everything else from the rule is discarded.
	 * The syntax node also contains a reference to the rule that created
	 * it. No ownership of the rule is taken, it is therefore important
	 * that the source rules remain in memory even though no more parsing
	 * is to be performed.
	 */
	class SyntaxNode : public Printable, NoCopy {
	public:
		SyntaxNode(const SyntaxOption *option);

		~SyntaxNode();

		// Content for a variable.
		struct Var {
			SyntaxVariable *value;
			vector<String> params;
		};

		// Member function invocation.
		struct Invocation {
			String member;
			Var val;
		};


		// Add a value to the syntax node. Throws an error on failure.
		void add(const String &var, const String &val, const vector<String> &params, const SrcPos &pos);
		void add(const String &var, SyntaxNode *node, const vector<String> &params, const SrcPos &pos);

		// Add a method invocation to the syntax node. Throws an error on failure.
		void invoke(const String &member, const String &val, const vector<String> &params, const SrcPos &pos);
		void invoke(const String &member, SyntaxNode *node, const vector<String> &params, const SrcPos &pos);

		// Find the entry in the map for the variable. Returns null if the variable does not exist.
		const Var *find(const String &name) const;

		// Reverse all arrays in this node (not recursive).
		void reverseArrays();

		// The syntax option that resulted in this node.
		const SyntaxOption *const option;

		// Get invocations.
		const vector<Invocation> &invocations;

	protected:
		virtual void output(wostream &to) const;

	private:
		// All bound variables of this syntax node.
		typedef map<String, Var> VarMap;
		VarMap vars;

		// Method invocations, in order.
		vector<Invocation> mInvocations;

	};

	wostream &operator <<(wostream &to, const SyntaxNode::Var &v);
	wostream &operator <<(wostream &to, const SyntaxNode::Invocation &f);

	/**
	 * Error thrown when a variable in a syntax rule is assigned twice.
	 */
	class SyntaxVarAssignedError : public CodeError {
	public:
		SyntaxVarAssignedError(const SrcPos &pos, const String &var) : CodeError(pos), var(var) {}

		inline String what() const { return ::toS(where) + L": Variable already assigned: " + var; }
	private:
		String var;
	};

}
