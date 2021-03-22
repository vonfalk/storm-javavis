// Include the specific synthesizers we use. It is a pain to exclude all but the one we need.

#ifndef DEBIAN_BUILD
#include "mpg123/src/libmpg123/synth_real.c"
#include "mpg123/src/libmpg123/synth_s32.c"
#endif
