#include "stdafx.h"

/**
 * Simple GC tests that can be used during the creation of a new GC so that large parts of the
 * compiler does not need to be rebiult so often during development.
 */
int main() {
	storm::Gc gc(10*1024*1024, 1000);


	return 0;
}
