// Detects if the cursor hits the edge
// Author: Max Schwarz <Max@x-quadraht.de>

#include "edgewindow.h"

EdgeWindow::EdgeWindow(Display* display, Direction dir)
 : m_display(display)
 , m_direction(dir)
{
	// Get screen dimensions
	Screen* screen = DefaultScreenOfDisplay(display);
	int sw = WidthOfScreen(screen);
	int sh = HeightOfScreen(screen);
	
	int x, y, w, h;
	
	switch(dir)
	{
		case D_LEFT:   x = 0;    y = 0;    w = 1;    h = sh;   break;
		case D_RIGHT:  x = sw-1; y = 0;    w = 1;    h = sh;   break;
		case D_TOP:    x = 1;    y = 0;    w = sw-2; h = 1;    break;
		case D_BOTTOM: x = 1;    y = sh-1; w = sw-2; h = 1;    break;
		case D_UNKNOWN: fatal("D_UNKNOWN in EdgeWindow");
	}
	
	XSetWindowAttributes attr;
	attr.override_redirect = True;
	attr.event_mask = EnterWindowMask;
	
	unsigned long valuemask = CWOverrideRedirect | CWEventMask;
	
	m_window = XCreateWindow(display, DefaultRootWindow(display),
	                 x, y, w, h, 0, CopyFromParent, InputOnly, CopyFromParent, valuemask, &attr);
	
	XMapWindow(display, m_window);
}

EdgeWindow::~EdgeWindow()
{
	XDestroyWindow(m_display, m_window);
}