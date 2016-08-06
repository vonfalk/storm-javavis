#pragma once
#include "Core/Object.h"
#include "Core/TObject.h"
#include "Core/Gen/CppTypes.h"
#include "ValueArray.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Represents all instantiations of a template class available to C++.
	 *
	 * Note: This object needs to be thread-safe in the find() function.
	 */
	class TemplateList : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		typedef CppTemplate::GenerateFn GenerateFn;

		// Create.
		TemplateList(GenerateFn generate);

		// Find an instantiaton. Generates it if neccessary.
		Type *find(Nat *elems, Nat count);

	private:
		// Remember what we generated.
		UNKNOWN(PTR_GC) GenerateFn generateFn;

		// A node in the list.
		struct Node;

		// Multi-level list of generated templates.
		UNKNOWN(PTR_GC) Node *root;

		// Find a template specialization inside the structure.
		Type *findAt(Nat *elems, Nat count, Node *node, Nat at);

		// Insert a type.
		void insertAt(Nat *elems, Nat count, Type *insert, Node *&node, Nat at);

		// Allocate a node of a specific size.
		Node *allocNode(Nat count);
		Node *allocNode(const Node *original, Nat newCount);

		// Generate a new template.
		Type *generate(Nat *elems, Nat count);
	};

}
