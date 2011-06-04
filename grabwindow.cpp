// This window grabs the mouse & handles movement
// Author: Max Schwarz <Max@x-quadraht.de>

#include "grabwindow.h"

#include "utils.h"

#include <X11/Xatom.h>

#include <stdio.h>
#include <string.h>

GrabWindow::GrabWindow(Display* display)
 : m_display(display)
{
	Window root = DefaultRootWindow(display);
	
	// Get screen dimensions
	Screen* screen = DefaultScreenOfDisplay(display);
	int w = WidthOfScreen(screen);
	int h = HeightOfScreen(screen);
	
	XSetWindowAttributes attr;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect = True;
	attr.cursor = createBlankCursor(display, root);
	
	attr.event_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	
	m_window = XCreateWindow(display, root, 0, 0, w, h,
	                0, 0, InputOnly, CopyFromParent, CWDontPropagate | CWEventMask | CWOverrideRedirect | CWCursor, &attr);
	if(m_window == None)
		fatal("Could not create window");
	
	const char* name = "unity_grab";
	XChangeProperty(m_display, m_window, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (const unsigned char*)name, strlen(name));
	
	m_center[0] = w/2;
	m_center[1] = h/2;
}

GrabWindow::~GrabWindow()
{
	XDestroyWindow(m_display, m_window);
}

void GrabWindow::grab()
{
	// Show grab window
	XMapRaised(m_display, m_window);
	
	// Grab pointer
	if(XGrabPointer(m_display, m_window, False, PointerMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, m_window, None, CurrentTime) != GrabSuccess)
		fatal("Failed to grab pointer");
	
	if(XGrabKeyboard(m_display, m_window, False, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
		fatal("Failed to grab keyboard");
	
	resetCursor();
}

void GrabWindow::release()
{
	XUngrabPointer(m_display, CurrentTime);
	XUngrabKeyboard(m_display, CurrentTime);
	XUnmapWindow(m_display, m_window);
}

void GrabWindow::resetCursor()
{
	XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, m_center[0], m_center[1]);
}

void GrabWindow::processMotion(const XMotionEvent* event, int* dx, int* dy)
{
	*dx = event->x - m_center[0];
	*dy = event->y - m_center[1];
	
	resetCursor();
}

