Debugging
==========

There are a few utilities in the C++ code that can be used to make debugging easier. Since Storm
generates code, it is harder to use some functionality in C++ debuggers as usual. For example, stack
traces from within debuggers are unaware of the function names for generated code. Therefore, it is
not possible to step through Storm code in source mode, or set breakpoints in Storm code easily.


Breakpoints
------------

The function `storm::dbgBreak()` is useful for breaking the program at an arbitrary point. It is
exposed to Storm as `core.debug.dbgBreak()`, which makes it useful in Storm code as well. On
Windows, it calls the function `DebugBreak()`, which breaks into a debugger if you have one
installed. On Linux, it raises the signal SIGINT, which is easily seen inside GDB.


Stack traces
-------------

To print a stack trace, call the function `storm::stackTrace()` from C++ or
`core.debug.stackTrace()`. This stack trace will properly print the names of functions inside Storm
in addition to the functions from C++. If you are writing code in a shared library,
`storm::stackTrace` is not accessible, in which case one can use `::dumpStack()` in
`Utils/StackTrace.h` instead.


Signals on Linux
-----------------

There are a few additional things one need to consider when debugging Storm on Linux. Since the
garbage collector uses signals to synchronize scanning of the thread's stacks (SIGXFSZ and SIGXCPU),
GDB will halt the program execution when that happens. Furthermore, the garbage collector utilizes
memory protection features in Linux to detect when data has been read or written, which can be used
to avoid doing unnecessary work. The downside of this is that SIGSEGV signals are generated and
handled during normal program operation. Thus, SIGSEGV can not be used to find memory errors. To
simplify debugging, Storm installs another signal handler for SIGSEGV that is executed if the
garbage collector determined that the SIGSEGV represented a genuine segmentation fault. This handler
simply raises SIGINT so that it is easy to distinguish between segmentation faults and normal GC
operation. However, for various reasons, the final SIGINT may be handled by a thread different from
the thread where the segmentation fault occurred.

To make GDB ignore the signals used with the garbage collector, put the following in your `.gdbinit`
file, or type them into the interactive prompt when you start GDB:

```
handle SIGXFSZ nostop noprint
handle SIGXCPU nostop noprint
handle SIGSEGV nostop noprint
```


Generated code
---------------

If you are developing languages or language extensions, it can sometimes be convenient to see the
final machine code that is generated instead of the intermediate representation. This can be done by
the following C++ code:

```
// See the transformed machine code (l is a Listing):
PLN(engine().arena()->transform(l, null));

// See the binary representation as well:
PLN(new (engine()) Binary(engine().arena(), l, true));
```