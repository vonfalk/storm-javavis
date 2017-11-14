Storm
========

?Button(md://Introduction/Downloads)Download Storm?

Storm is a work in progress language that focuses on extensibility on many levels. Read more about
it [here](md://Storm/).

Downloads
----------

Go to the download page by clicking the button to the right. Precompiled binaries for Windows
(32-bit) and Linux (64-bit, X86-64) are provided.

To run the compiler, simply unpack the archive file and run `Storm` (`Storm.exe` on Windows), and
the top loop for Basic Storm should start. The compiler does not need any external libraries aside
from the C and C++ standard libraries. The Windows version uses `dbghelp.dll`, which is included
with Windows, to pretty-print stack traces.


Documentation
--------------

Start by going to [Introduction](md://Introduction/) for information on how to download and install
Storm, and for some concrete examples of what Storm is capable of.

The documentation provided here aims to provide an understanding of the language itself. It will not
generally discuss specific APIs or the standard library in depth. For documentation on specific
functions, objects or packages, please refer to the built-in documentation in Storm.
When using Basic Storm, import the package `lang.bs.macro` and use the `explore{name}` syntax to
output information about packages, types, functions or most other named things in the system. Sadly,
the built-in documentation is not finished yet.

For information on licenses used in the system, type `licenses` in the Basic Storm top loop, or call
`core.info.licenses` from your code. Note that while Storm is licensed under the LGPL version 2.1,
Storm uses the [Memory Pool System](http://www.ravenbrook.com/project/mps/) from Ravenbrook Ltd. for
memory management, which requires the source code for all programs using the MPS being freely
available unless another license is acquired. Furthermore, it is possible to integrate other garbage
collectors into Storm if the MPS license is an issue.

More information on the language server can be found at [http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847](http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847)

Contact
--------

If you have any questions or requests regarding Storm, please contact me at [info@storm-lang.org](info@storm-lang.org).
