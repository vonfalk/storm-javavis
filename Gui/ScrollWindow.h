#pragma once
#include "Container.h"

namespace gui {

	/**
	 * A window containing a single other window of some sort, and scrolls the contents of that
	 * window.
	 *
	 * By default, only vertical scrolling is enabled.
	 */
	class ScrollWindow : public ContainerBase {
		STORM_CLASS;
	public:
		// Create, indicating what is stored inside.
		STORM_CTOR ScrollWindow(Window *inside);

		// Create and set the minimum size.
		STORM_CTOR ScrollWindow(Window *inside, Size minSize);

		// Get/set the minimum size.
		virtual void STORM_ASSIGN minSize(Size sz);
		virtual Size STORM_FN minSize();

		// Enable/disable horizontal scrolling.
		virtual void STORM_ASSIGN horizontal(Bool v);
		Bool STORM_FN horizontal() const { return hScroll; }

		// Enable/disable vertical scrolling.
		virtual void STORM_ASSIGN vertical(Bool v);
		Bool STORM_FN vertical() const { return vScroll; }

		// Called when our parent is created.
		virtual void parentCreated(nat id);

		// Called when we've been destroyed.
		virtual void windowDestroyed();

		// Get the window scrolled.
		Window *STORM_FN content() const { return child; }

		// Called when resized.
		virtual void STORM_FN resized(Size size);

#ifdef GUI_GTK
		// Add a child widget to the layout here.
		virtual void addChild(GtkWidget *child, Rect pos);

		// Move a child widget.
		virtual void moveChild(GtkWidget *child, Rect pos);
#endif
#ifdef GUI_WIN32
		// Handle messages.
		virtual MsgResult onMessage(const Message &msg);
#endif
	protected:
		// Create.
		virtual bool create(ContainerBase *parent, nat id);

	private:
		// Contained window.
		Window *child;

		// Stored minimum size.
		Size minSz;

		// Horizontal/vertical scroll.
		Bool hScroll;
		Bool vScroll;

		// Set the size of the child window.
		void setChildSize(Size size);

#ifdef GUI_WIN32
		// Handle a scroll message.
		LRESULT onScroll(WPARAM wparam, int which);

		// Handle a mouse wheel scroll message.
		LRESULT onWheel(int delta, int which);

		// Set scroll info for a scroll bar.
		void setScrollInfo(int which, Float childSz, Float ourSz);

		// Update scrollbars.
		void updateBars(Size childSz);
#endif
	};

}
