Storm
========

?File(Download Storm)storm.zip?

Storm is a work in progress language that focuses on extensibility on many levels. Read more about
it [here](md://Storm/).

Downloads
----------

Storm is currently a work in progress, and it is not ready for any official releases yet. Therefore,
there is no version numbering for binaries and documentation at the moment. This will be introduced
when the language is closer to completion.

Currently, Storm only works on Windows, 32-bit. It will be ported to other platforms in the future,
but nothing is planned yet, as the main focus is to complete the language first. The download
contains an executable along with code required for Basic Storm to work properly. Furthermore
libraries for sound playback, image loading and basic GUI creation is included. The source code for
the project is available upon request to [info@storm-lang.org](mailto:info@storm-lang.org).

To run the compiler, simply unpack the zip-file and run `StormMain.exe`, and the top loop for Basic
Storm should start. The compiler does not need any external libraries. It is statically linked to
the C runtime, and only depends on the Windows API and `dbghelp.dll`. `dbghelp.dll` is included with
Windows, at least from Windows XP, so that should not be any problem. It even runs on Wine (tested
with Wine version 1.7.40 on Arch Linux).

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
