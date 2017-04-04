Getting Started
=================

This portion of the documentation serves as a guide for getting Storm up and running on your
machine, as well as illustrating some of the important concepts in Storm by taking a closer look at
the examples included in the download.


Downloading and Installing Storm
----------------------------------

First, download Storm from [here](storm.zip) and unpack the archive somewhere convenient. The
archive contains two folders and one executable file:

* `doc` contains the source files for this documentation (in markdown format).
* `root` is the directory that Storm uses as the root package when started. The contents of this
  directory is described in greater detail below.
* `Storm.exe` is the main executable for Storm. Start it normally to open an interactive command
  prompt where Storm code can be executed. You can also pass command line parameters to Storm to
  make it do other things than running the interactive command prompt. Use `Storm --help` or
  `Storm -?` to see a list of supported options.

The `root` directory contains the packages loaded by Storm at startup. This is where all Storm code
lives, each directory here corresponds to a package in Storm. Some of these are described below:

* `demo`
* `lang`
* `graphics`
* `sound`
* `ui`
* `present`
