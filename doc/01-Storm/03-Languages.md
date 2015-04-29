Languages
===========

Languages in Storm are synonymous with file extensions. When Storm tries to load the contents of a
package, it examines the different file extensions present and creates a subclass of the
`core.lang.PkgReader` for each of the files. These readers are expected to be located at
`lang.<ext>.Reader`, where `<ext>` is the file extension. When all `Readers` are created, Storm asks
each of them to load various parts of the language. This is to allow the different languages to find
types declared in the same package but inside a different language.

Because of this, it is quite easy to create a new languae. Simply create a new package named
`lang.a`, and Storm will load `.a`-files using your newly created `Reader`.

Loading
--------

Code is loaded in the following steps:

1. load syntax
2. load types
3. finalize types
4. load functions

Step 3 is used to set up inheritance for types and other tasks that requires information about other
types present in the same package.

Each `Reader` is handed a set of all files of the same type that are found, since some files may
depend on each other. However, in the case that no files overlap, Storm provides an implementation
of `Reader` that is based on individual files instead of a set of files. To use it, simply inherit
your `Reader` from `core.lang.FilesReader`, and implement the `createFile` member to create a
`core.lang.FileReader` instance for each file. The `FilesReader` will then take care of forwarding
calls for the loading progress to all `FileReader`s.