#include <stdio.h>
#include <stdlib.h>

#include <X11/cursorfont.h> // XC_pencil
#include <X11/X.h>
#include <X11/Xlib.h>

int
main(int argc, char** argv)
{
	Display* dpy = XOpenDisplay(NULL);
	if (!dpy)
		return EXIT_FAILURE;
	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	unsigned long black = BlackPixel(dpy, screen);
	unsigned long white = WhitePixel(dpy, screen);

	Window win;
	win = XCreateSimpleWindow(dpy, root, 100, 100, 100, 100, 10, black, white);

	// Display the window
	XMapWindow(dpy, win);

	// Use pencil as cursor
	XDefineCursor(dpy, win, XCreateFontCursor(dpy, XC_pencil));

	// Sync
	XSync(dpy, 0);

	int x, y;
	unsigned int mask;

	while (1) {
		// Throw-away, cannot pass NULL
		int i;
		Window w;

		// Get pointer location
		XQueryPointer(dpy, win, &w, &w, &i, &i, &x, &y, &mask);
		// printf("x: %d, y: %d, mask: %u\n", x, y, mask);

		int left_click  = mask & (1 << 8);
		int right_click = mask & (1 << 10);
		if (left_click)
			printf("x: %d, y: %d\n", x, y);

		if (right_click)
			break;
	}

	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
