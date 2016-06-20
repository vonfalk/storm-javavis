#pragma once
#include "Variable.h"

/**
 * Interface that acts like a namespace, ie. it stores functions and variables where they belong.
 */
class Namespace {
public:
	virtual void add(const Variable &v) = 0;
};
