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
rendered to the display.
