#include "stdafx.h"
#include "TemplateList.h"
#include "Engine.h"

namespace storm {

	/**
	 * Compatible with GcArray. We're also storing a pointer to another node in here, so 'elements'
	 * will always be one too large. Therefore, we store the 'correct' count inside 'count'.
	 */
	struct TemplateList::Node {
		size_t elements;
		size_t count;

		// If we're done.
		Type *done;

		// Instantiations here.
		Node *next[1];
	};

	/**
	 * Gc description.
	 */
	static const GcType nodeType = {
		GcType::tArray,
		null,
		null,
		sizeof(void *), // Element size
		1, // One pointer in each element...
		{ 0 }, // ... at offset 0.
	};

	TemplateList::TemplateList(GenerateFn generate) : generateFn(generate) {}

	Type *TemplateList::find(Nat *elems, Nat count) {
		TODO(L"Lock me!");

		Type *found = findAt(elems, count, root, 0);
		if (!found) {
			found = generate(elems, count);
			insertAt(elems, count, found, root, 0);
		}

		return found;
	}

	Type *TemplateList::findAt(Nat *elems, Nat count, Node *node, Nat at) {
		if (!node)
			return null;

		if (count == at)
			return node->done;

		Nat now = elems[at];
		if (node->count <= now)
			return null;

		return findAt(elems, count, node->next[now], at + 1);
	}

	void TemplateList::insertAt(Nat *elems, Nat count, Type *insert, Node *&node, Nat at) {
		if (count == at) {
			// Insert it here!
			if (!node)
				node = allocNode(0);

			node->done = insert;
			return;
		}

		Nat now = elems[at];

		// Create a new node?
		if (!node)
			node = allocNode(now + 1);

		// Increase size of existing node?
		if (node->count <= now)
			node = allocNode(node, now + 1);

		insertAt(elems, count, insert, node->next[now], at + 1);
	}

	TemplateList::Node *TemplateList::allocNode(Nat count) {
		Node *n = (Node *)engine().gc.allocArray(&nodeType, count + 1);
		n->count = count;
		return n;
	}

	TemplateList::Node *TemplateList::allocNode(const Node *original, Nat count) {
		Node *mem = allocNode(count);
		mem->done = original->done;
		for (nat i = 0; i < min(count, original->count); i++)
			mem->next[i] = original->next[i];
		return mem;
	}

	Type *TemplateList::generate(Nat *elems, Nat count) {
		Engine &e = engine();
		ValueArray *types = new (e) ValueArray();
		types->reserve(count);

		for (nat i = 0; i < count; i++) {
			Type *t = e.cppType(elems[i]);
			assert(t, L"Type with id " + ::toS(elems[i]) + L" not found.");
			types->at(i) = Value(t);
		}

		Type *r = (*generateFn)(types);
		assert(r, L"Invalid template usage for types " + ::toS(types));
		return r;
	}

}
