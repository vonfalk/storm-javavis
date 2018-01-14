Language Server
==================

In order to make interaction with new languages and language extensions easier, Storm provides a
language server. This means that it is possible to run Storm in the background while developing
programs so that the editor is able to ask Storm about tasks such as syntax highlighting and
indentation. This, in turn allows the editor to provide good feedback regardless of whether the
language is supported by the editor or not.


In order to use the language server, a plugin for your editor is required. At the moment, a plugin
for Emacs is maintained as a reference implementation. It is included when downloading Storm as
`storm-mode.el`. If no plugin is yet available for your editor of choice, the protocol is described
(here)[md://Storm/Language_Server/Protocol], which makes it possible to build one without too much
trouble.

The goal is to extend the protocol to allow sending commands to the top-loop, compiling parts of the
system etc. through the language server. We're not there yet, but we're getting there!

For a more detailed description of the language server, look at the master thesis that is formed the
foundation of the language server:
[http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847](http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847)

The Emacs plugin
-------------------

The Emacs plugin works with Emacs version 25.0 or higher. To use it, simply load it from your
`.emacs`-file, using `(load "~/path/to/storm-mode.el")`. Aside from loading the plugin, you need to
set two variables which tell the plugin where to find Storm, and where the directory containing the
standard library are located (the root directory included in the download). This is done using
`(setq storm-mode-compiler "~/path/to/Storm.exe")` and `(setq storm-mode-root "~/path/to/root/")`.

After the set up, you can test the plugin by doing `M-x storm-start`, which starts Storm in the
background. You can see the output from Storm by examining the buffer `*compilation*`, which is
hidden by default. If everything works as expected, you should see the text `Language server started.`
there. The plugin will start the Storm process as needed, so you do not have to run `storm-start`
manually in the future. If you wish to stop the language server, run `M-x storm-stop`.

After verifying the setup, you can start using `storm-mode` for files known to Storm. This is done,
like other modes in Emacs, by doing `M-x storm-mode`. If the current file is supported by Storm, you
should see syntax highlighting from Storm while editing the file. If you examine the `*compilation*`
buffer, you should see a message noting that the file has been opened by Storm, or an appropriate
error message on failure.

While it is possible to use `storm-mode` on individual files, or add `storm-mode` to the
`auto-mode-alist` variable, which is usually done with the standard Emacs modes, it is sometimes
convenient to use Storm for all supported files globally. This can be done by enabling
`global-storm-mode`. This mode examines all currently open buffers and all newly opened buffers for
files supported by Storm and uses `storm-mode` for them in that case. This supersedes any other
modes that are applicable for those files. Enabling the global mode can be done by invoking
`M-x global-storm-mode`, which toggles between using and not using the global mode. If you wish
to enable or disable it permanently, put `(global-storm-mode t)` or `(global-storm-mode nil)` in
your `.emacs`-file.

While using `storm-mode`, you can use the following keyboard shortcuts to interact with Storm:
* `C-c u` or `storm-re-color`: update the syntax coloring of the entire file again. This is useful if, for
  some reason, Storm or Emacs missed coloring some characters, or got out of sync. This
  merely transmits the syntax highlighting for the entire file once again, and does not
  attempt to re-parse the file.
* `C-c r` or `storm-re-open`: similar to `storm-re-color`, but parses the file from scratch first.
  This is useful if Storm fails to see the correct parse of a buffer that previously contained many
  or large errors. It could also be required if some edit operations have been lost while interacting
  with the buffer. Usually this rarely happens.

Indentation in Storm-mode respects `tab-width` for indentation levels.
