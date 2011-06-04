// (X11) utils

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* DIRECTION_STR[] = {
	"Left",
	"Right",
	"Top",
	"Bottom"
};

bool mask_bit_set(unsigned char* mask, int mask_len, int bit)
{
	int byte_num = bit / 8;
	
	if(byte_num >= mask_len)
		return false;
	
	return mask[byte_num] & (1 << (bit % 8));
}

Cursor createBlankCursor(Display* dpy, Window root)
{
	// this seems just a bit more complicated than really necessary

	// get the closet cursor size to 1x1
	unsigned int w, h;
	XQueryBestCursor(dpy, root, 1, 1, &w, &h);

	// make bitmap data for cursor of closet size.  since the cursor
	// is blank we can use the same bitmap for shape and mask:  all
	// zeros.
	const int size = ((w + 7) >> 3) * h;
	char* data = new char[size];
	memset(data, 0, size);

	// make bitmap
	Pixmap bitmap = XCreateBitmapFromData(dpy, root, data, w, h);

	// need an arbitrary color for the cursor
	XColor color;
	color.pixel = 0;
	color.red   = color.green = color.blue = 0;
	color.flags = DoRed | DoGreen | DoBlue;

	// make cursor from bitmap
	Cursor cursor = XCreatePixmapCursor(dpy, bitmap, bitmap,
								&color, &color, 0, 0);

	// don't need bitmap or the data anymore
	delete[] data;
	XFreePixmap(dpy, bitmap);

	return cursor;
}

void fatal(const char* msg, ... )
{
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	perror(" ");
	va_end(ap);
	exit(1);
}
