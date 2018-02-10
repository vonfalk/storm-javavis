#pragma once
#include "Auto.h"
#include "Tokenizer.h"

/**
 * Contains documentation for types, functions and variables in the C++ code.
 *
 * TODO: Add support for referring to a 'block' within the source.
 */
class Doc : public Refcount {
public:
	// Create documentation from a comment.
	Doc(const Comment &src);

	// Create documentation from a string.
	Doc(const String &src);

	// Contents of the comment.
	String v;

	// Where is the documentation stored in the documentation file? Zero is reserved as 'not present'.
	nat fileId;

	// Get the id, but check so that it is valid.
	nat id() const;
};

wostream &operator <<(wostream &to, const Doc &doc);
