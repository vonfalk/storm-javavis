#pragma once

#ifdef DEBIAN_BUILD
#include <FLAC/stream_decoder.h>
#else
#include "FLAC/stream_decoder.h"
#endif
