#pragma once

// Macro for getting the offset of a member within a structure.
#define OFFSET_OF(baseType, memberName) \
	size_t(&(((baseType *)0)->memberName))

// Macro for getting a pointer to the containing object
// from a pointer to a member within the structure.
#define BASE_PTR(baseType, pointer, member) \
	((baseType *)(((byte *)(pointer)) - OFFSET_OF(baseType, member)))

// Macro for getting an offset into a chunk of memory.
#define OFFSET_IN(ptr, offset, type) \
	(*(type *)(((byte *)(ptr)) + (offset)))

// Example:
// struct Foo { int a, b, c; };
// Assume we have an int * we know is pointing to "b" in "Foo". We can then use
// BASE_PTR(Foo, <ptr>, b); to get the pointer to "Foo" again.
// void *fooPtr = &foo;
// OFFSET_IN(fooPtr, OFFSET_OF(Foo, b), int) = 1;
