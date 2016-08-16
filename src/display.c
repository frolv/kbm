#include "kbm.h"
#include "display.h"

#include <stdlib.h>

#ifdef __linux__
#include <X11/Xlib.h>
#endif

void init_display()
{
	Display *dpy;
	Window win;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "error: failed to open display\n");
		exit(1);
	}
	win = XCreateWindow(dpy, DefaultRootWindow(dpy),
			10, 10, 200, 200, 0, 0, InputOnly, CopyFromParent, 0, NULL);

	XCloseDisplay(dpy);
}
