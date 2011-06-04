// (X11) utils

#ifndef UTILS_H
#define UTILS_H

#include <X11/Xlib.h>

#include <stdarg.h>

enum Direction
{
	D_LEFT = 0,
	D_RIGHT = 1,
	D_TOP = 2,
	D_BOTTOM = 3,
	D_UNKNOWN = 4
};

extern const char* DIRECTION_STR[];

inline Direction oppositeDirection(Direction dir)
{
	switch(dir)
	{
		case D_LEFT:   return D_RIGHT;
		case D_RIGHT:  return D_LEFT;
		case D_TOP:    return D_BOTTOM;
		case D_BOTTOM: return D_TOP;
		default:       return D_UNKNOWN;
	}
}

/**
 * @brief Create a blank cursor
 * 
 * That's what it takes to hide the cursor in X11...
 * */
Cursor createBlankCursor(Display* dpy, Window root);

/**
 * @brief Check if bit is set in bitmask
 * */
bool mask_bit_set(unsigned char* mask, int mask_len, int bit);

/**
 * @brief Exit with fatal error
 * */
void fatal(const char* msg, ...);

#endif
