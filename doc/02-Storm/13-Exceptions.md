Exceptions
==========

All exceptions in Storm inherit from the class `core.Exception`, which is a class-type. The
`core.Exception`-class is abstract, and expects derived classes to override the `message(StrBuf)`
function, which is to provide a custom message for the exception when printed.

Exceptions do not record a stack trace by default since this is a fairly expensive operation. The
`core.Exception` class contains a `saveTrace` function that records a stack trace into the exception
that will be printed by the default `toS` implementation. Many of the built-in exceptions in Storm
represent events that are fatal, and are thereby not expected to be caught and recovered from and
therefore collect stack-traces by default to aid debugging (in these cases the performance impact is
less important compared to debuggability). Therefore, classes inheriting from these classes do not
need to call `saveTrace` again.

Storm defines the following generic exception classes:
- `NotSupported`: Thrown from interfaces where the requested operation is not supported in this case.
- `InternalError`: Internal errors from the compiler or the runtime system.
- `RuntimeError`: Generic runtime errors originating from incorrect usage of interfaces in the system.
- `StrError`, `MapError`, `SetError`, `ArrayError`: Errors thrown by the standard containers if they
  are used incorrectly (e.g. accessing elements that don't exist).
