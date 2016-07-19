#pragma once
#include "Variable.h"
#include "Function.h"

/**
 * Interface that acts like a namespace, ie. it stores functions and variables where they belong.
 */
class Namespace {
public:
	virtual void add(const Variable &v) = 0;
	virtual void add(const Function &f) = 0;
};
