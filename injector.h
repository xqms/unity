// Fake X11 injection
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef INJECTOR_H
#define INJECTOR_H

#include <X11/Xlib.h>

class Injector
{
	public:
		Injector(Display* display);
		
		void injectMotionAbsolute(int x, int y);
		void injectMotionRelative(int dx, int dy);
		void injectButtonEvent(int button, bool press);
		void injectKeyEvent(int keycode, bool press);
	private:
		Display* m_display;
};

#endif
