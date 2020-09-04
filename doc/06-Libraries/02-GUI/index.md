GUI
====

Storm includes a GUI library for creating graphical applications. This library uses an additional
thread to handle UI events, and another thread to handle repaints. UI elements are based around the
`Window` class. A `Window` is a rectangle on the screen, maybe obscured by another window. Each
window, except for `Frames`, have a parent window in which they are contained. `Frames` are special,
as they are the top-level windows. These are usually decorated by the operating system with a border
so that the user can move and close the window easier.


Repaints are handled by a different thread, named `Render`. This thread is in charge of re-painting
all windows when they need to. Repaints are done either when the operating system requests a
repaint, when you call `Window.repaint()`, or if you want your window to be redrawn at each frame
rendered to the display. Actual drawing is handled by a `Painter` class, which is attached to a
window using `painter = MyPainter`. The `draw` member of the painter class will then be called
whenever the window needs to be redrawn.

DPI awareness (on Windows)
-------------

The GUI library is DPI aware on Windows. This means that the contents of windows might be rendered
at a different resolution than what was requested by the program according to the user's
preferences. This is handled transparently by the GUI library by transforming any coordinates
supplied accordingly. This means that programs using the Gui library may assume that coordinates and
sizes are always relative to a 96 DPI resolution (the standard size in Windows), and leave the rest
to the GUI library. This does, however, have some implications. Programs can not assume that one
"coordinate step" represents one pixel, neither when drawing nor when positioning components in a
window. Furthermore, as the physical size of fonts do not always scale linearly with the font size,
long chunks of text may sometimes appear to take more space, and sometimes less. When using
automatic layout, this is generally handled gracefully anyway.

On Linux, Storm relies on the support in Gtk+.

Rendering backends (on Linux)
-------

The GUI library supports multiple rendering backends on Linux (all supported through Cairo) for
content rendered in Storm (i.e. not the widgets rendered by Gtk+ itself). By default, the GUI
library utilizes the backend used by Gtk+, which should be sufficient in most cases. It is, however,
possible to fall back to software rendering entirely, or to use the OpenGL rendered in Cairo. This
is selected by setting the environment variable `STORM_RENDER_BACKEND` to one of the following values:

- `gtk` (the default): Use the same backend Gtk+ uses. On X11, this usually involves using XRender
  extensions to accelerate the graphics (at least according to the Cairo manual), and has the
  benefit that copying the back-buffer to the window is fast as all content resides in the same
  location. On Wayland, the situation should be similar, but a different (usually hardware
  accelerated somehow) backend is used.
- `sw` or `software`: Use the software rendering backend in Cairo. This is useful as a failsafe
  default if the standard modes cause the video driver to crash, or cause other issues.
- `gl`: Use the OpenGL backend of Cairo. This potentially increases rendering performance a bit, but
  usually requiures copying the rendered image to the window through the CPU, which can be slow. It
  is possible that the default backend is faster as well, as the OpenGL backend is currently marked
  experimental in Cairo. Furthermore, the OpenGL backend has been observed to cause some video
  drivers to crash (e.g. the iris driver for Intel cards).


Layout
-------

The GUI library is integrated with the [layout library](md://Libraries/Layout). The library provides
a convenient way of declaring windows with layout in Basic Storm as follows:

```
window MyWin {
    layout Grid {
        wrapCols: 2;
        expandCol: 0;
        Button a("A") {}
        Button b("B") { rowspan: 2; }
        Button c("C") {}
        nextLine;
        Button d("D") {}
        Button e("E") {}
        Button f("F") { row: 3; col: 2; }
        Button g("G") { row: 4; col: 1; colspan: 2; }
    }

    init() {
        init("My window", Size(200, 200));
        create();
    }
}
```

When using the `window` keyword instead of `class`, it is possible to specify a `layout` block
inside the class. By default these classes inherit from `ui.Frame`, but this can be overridden using
the keyword `extends` as usual.

The layout language is also extended by the ui library to allow declaring variables inside the
layout. These variables will become member variables inside the class itself, and the initialization
will be performed inside the constructor. Since the initialization will be performed inside the
constructors of the class, it is possible to use any local variables declared before the `init`
block in the constructor inside the initialization, even though this easily gets confusing.


Notes
------

Below are some things to consider when developing applications with the UI library:

- When drawing text, do not attempt to reduce its size too much by scaling it with a
  transformation. If text is drawn in a too small size (at least vertical size, most likely also
  horizontal), the underlying text libraries do not behave very well. Pango (used on Linux) seems to
  enter an infinite loop if attempting to draw with a vertical scale factor of zero (possibly also
  very small scales). DirectWrite (used on Windows) does not enter an infinite loop, but seems to
  fail internally with a FileFormatException in similar circumstances, which causes sporadic
  performance irregularities during rendering. Limiting the scale factor to a minimum of 0.05 seems
  to work sufficiently well on both platforms, but for small font sizes this might be too small.
