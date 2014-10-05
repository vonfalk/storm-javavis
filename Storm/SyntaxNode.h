#pragma once
#include "SyntaxVariable.h"
#include "SyntaxRule.h"

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
		SyntaxNode(const SyntaxRule *rule);

		~SyntaxNode();

		// Add a value to the syntax node. Throws an error on failure.
		void add(const String &var, const String &val);
		void add(const String &var, SyntaxNode *node);

		// Find the entry in the map for the variable.
		SyntaxVariable *find(const String &name, SyntaxVariable::Type type);

	protected:
		virtual void output(wostream &to) const;

	private:
		// All bound variables of this syntax node.
		typedef map<String, SyntaxVariable*> VarMap;
		VarMap vars;

		// The syntax rule that resulted in this node.
		const SyntaxRule *srcRule;

		// Find the type to use for this variable.
		SyntaxVariable::Type typeOf(const String &name, bool isString);
	};

}
