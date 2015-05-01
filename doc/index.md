Storm
========

Storm is a work in progress language that focuses on extensibility on many levels. Read more about
it [here](md://Storm/).

Documentation
--------------

The documentation provided here aims to provide an understanding of the language itself. It will not
generally discuss specific API:s or the standard library in depth. For documentation on specific
functions, objects or packages, please refer to the built-in documentation in Storm.

When using Basic Storm, import the package `lang.bs.macro` and use the `explore{name}` syntax to
output information about packages, types, functions or most other named things in the system. Sadly,
the built-in documentation is not finished yet.

Downloads
----------

Storm is currently a work in progress, and it is not ready for any official releases yet. Therefore,
there is no version numbering for binaries and documentation at the moment. This will be introduced
when the language is closer to completion.

Currently, Storm only works on Windows, 32-bit. It will be ported to other platforms in the future,
but nothing is planned yet, the main focus is to complete the language. The download contains an
executable along with demo and test code and a sample implementation of Brainfuck. Source code of
the compiler itself is not availiable at the moment, but it will probably be released in the future.

[Download Storm](storm.zip)

To run the compiler, simply unpack the zip-file and run `StormMain.exe`, and the top loop for Basic
Storm should start. The compiler does not need any external libraries except for "dbghlp.dll" and
maybe the Visual Studio 2008 runtime (should be statically linked). It even runs on Wine, except
that it crashes whenever an exception is thrown. My guess is that Wines implementation of
"dbghlp.dll" is either incomplete or nonexistent, but I have not yet investigated this problem.
