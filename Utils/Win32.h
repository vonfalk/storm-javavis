#pragma once
#include "Platform.h"

/**
 * Our version of including Windows.h, we're modifying the behavior a bit.
 */

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

// Sometimes it re-defines 'small' to 'char', no good!
#ifdef small
#undef small
#endif

// We need to remove min and max macros...
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#endif


#ifdef POSIX
#include <pthread.h>
#include <semaphore.h>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, const char *argv[]);

#endif
