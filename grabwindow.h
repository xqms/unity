// This window grabs the mouse & handles movement
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef GRABWINDOW_H
#define GRABWINDOW_H

#include <X11/Xlib.h>

class GrabWindow
{
	public:
		GrabWindow(Display* display);
		virtual ~GrabWindow();
		
		void grab();
		void release();
		void resetCursor();
		void processMotion(const XMotionEvent* event, int* dx, int* dy);
	private:
		Display* m_display;
		Window m_window;
		int m_center[2];
};

#endif
