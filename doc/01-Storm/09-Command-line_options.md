Command line options
=====================

By default, launching Storm `StormMain` from the command line launches the REPL for Basic
Storm. This behaviour can be changed by using different command-line flags.

To start a repl of another language, simply pass the name of that language on the command line, and
it will start. For example: `StormMain bs` starts the Basic Storm REPL.

To pass a string to the REPL of a language, use the flag `-c`. For example: `StormMain bs -c 1+2` sends
the expression `1+2` to the Basic Storm REPL.

To run a single function, use the `-f` flag. This function may not take any parameters, and not all
return types are supported. Example: `StormMain -f foo.main`.

To specify another root path, use `-r`. For example: `StormMain -r root/` launches Storm with
`root/` as the root directory. The default is `root/` relative to the directory of the executable
file.

