Downloads
===========

The current version of Storm is `?StormVersion?`, released on ?FileDate:storm.zip?. Only the latest release is
provided in binary form. Earlier releases have to be compiled from source. All releases are marked
in the Git repository by a tag with the name `release/<version>`.

Release notes for each release are found [here](md://Introduction/Downloads/Release_Notes) or in the
annotated release tags in the Git repository.

Binary releases
-----------------

- [Windows (32-bit)](storm.zip)
- [Linux (Debian, GCC 6.2.0 or later)](storm.tar.gz)


Source releases
----------------

The source code (licensed under GNU LGPL version 2.1) is freely available through Git at the following URL:

`git clone git://storm-lang.org/storm.git`

The repository has a few submodules. To fetch them as well, execute the following commands inside the repository:

`git submodule init`

`git submodule update`


If you get an error about an unreachable submodule, don't worry. That repository only contains some
test data for the language server which is not required.

To build Storm, you need `mymake`, available at [GitHub](http://github.com/fstromback/mymake) or
`git://storm-lang.org/mymake.git`. When you have installed mymake, compiling Storm is just `mm release`
to make a release build. During development, use `mm Main` or `mm Test` to build the
development version of the main entry point and the test suite respectively.
