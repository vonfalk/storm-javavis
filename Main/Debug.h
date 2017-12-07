#pragma once

#ifdef DEBUG

/**
 * Various debugging tools that impact Storm on a global level.
 */


// Monitor syscalls for EINTR? Some libraries do not like that the MPS uses signals to pause
// threads. This helps track down what is failing.
#define CHECK_SYSCALLS


#endif
