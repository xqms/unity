// Detects if the cursor hits the edge
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef EDGEWINDOW_H
#define EDGEWINDOW_H

#include <X11/Xlib.h>

#include "utils.h"

class EdgeWindow
{
	public:
		EdgeWindow(Display* display, Direction dir);
		virtual ~EdgeWindow();
		
		inline Direction direction() const
		{ return m_direction; }
		
		inline Window window() const
		{ return m_window; }
	private:
		Display* m_display;
		Direction m_direction;
		Window m_window;
};

#endif
