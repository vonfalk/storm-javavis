#pragma once
#include "Auto.h"
#include "Tokenizer.h"

class World;

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

	// Get our ID. Creates identifiers on demand.
	nat id(World &world);

private:
	// Our ID. 0 means 'not yet assigned', and is thereby not a valid ID.
	nat fileId;
};

wostream &operator <<(wostream &to, const Doc &doc);
