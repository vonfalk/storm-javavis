#pragma once

//Macro for getting the offset of a member within a structure.
#define OFFSET_OF(baseType, memberName) \
	int(&(((baseType *)0)->memberName))

//Macro for getting a pointer to the containing object
//from a pointer to a member within the structure.
#define BASE_PTR(baseType, pointer, member) \
	((baseType *)(((byte *)(pointer)) - OFFSET_OF(baseType, member)))

//Example:
//struct Foo { int a, b, c; };
//Assume we have an int * we know is pointing to "b" in "Foo". We can then use
//BASE_PTR(Foo, <ptr>, b); to get the pointer to "Foo" again.