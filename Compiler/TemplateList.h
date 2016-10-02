#pragma once
#include "Thread.h"
#include "Core/Gen/CppTypes.h"
#include "Template.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class NameSet;

	/**
	 * Represents all instantiations of a template class available to C++.
	 *
	 * Note: This object needs to be thread-safe in the find() function.
	 */
	class TemplateList : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		TemplateList(TemplateFn *t);

		// Add everything in here to NameSet.
		void addTo(NameSet *to);

		// Get what to add into.
		inline NameSet *addTo() const { return addedTo; }

		// Find an instantiaton. Generates it if neccessary.
		Type *find(Nat *elems, Nat count);

		// Run 'fn' for all named objects in here.
		typedef void (*NamedFn)(Named *);
		void forNamed(NamedFn fn);

	private:
		// Template we're representing.
		TemplateFn *templ;

		// A node in the list.
		struct Node;

		// Multi-level list of generated templates.
		UNKNOWN(PTR_GC) Node *root;

		// Added to a NameSet? If non-null, we shall add any newly-generated templates here.
		NameSet *addedTo;

		// Find a template specialization inside the structure.
		Type *findAt(Nat *elems, Nat count, Node *node, Nat at);

		// Insert a type.
		void insertAt(Nat *elems, Nat count, Type *insert, Node *&node, Nat at);

		// Allocate a node of a specific size.
		Node *allocNode(Nat count);
		Node *allocNode(const Node *original, Nat newCount);

		// Generate a new template.
		Type *generate(Nat *elems, Nat count);

		// For each.
		void forNamed(Node *at, NamedFn fn);

		// Add to a NameSet.
		void addTo(Node *at, NameSet *to);
	};

}
