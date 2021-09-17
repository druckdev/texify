#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>      // XLookupString(), XK_Escape (X11/keysymdef.h)
#include <X11/cursorfont.h> // XC_pencil

#include "texify.h"

Display* dpy;
Window win;
Atom wm_delete_msg;
Cursor pencil_cursor;
GC gc;

int
X_setup()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy)
		return 0;
	int screen  = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	unsigned long black = BlackPixel(dpy, screen);
	unsigned long white = WhitePixel(dpy, screen);

	win = XCreateSimpleWindow(dpy, root, 100, 100, 100, 100, 10, white, black);

	// Listen to key and button presses and cursor dragging (move while click)
	XSelectInput(dpy, win,
	             KeyPressMask | ButtonPressMask | ButtonReleaseMask |
	                     Button1MotionMask);
	// Listen for WM_DELETE_WINDOW message
	wm_delete_msg = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(dpy, win, &wm_delete_msg, 1);

	// Display the window
	XMapWindow(dpy, win);

	// Use pencil as cursor
	pencil_cursor = XCreateFontCursor(dpy, XC_pencil);
	XDefineCursor(dpy, win, pencil_cursor);

	// Sync
	XSync(dpy, False);

	// Setup drawing
	gc = XCreateGC(dpy, win, 0, NULL);
	XSetForeground(dpy, gc, white);

	return 1;
}

void
X_destroy()
{
	XFreeCursor(dpy, pencil_cursor);
	XFreeGC(dpy, gc);
	XCloseDisplay(dpy);
}

long
get_msec()
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return time.tv_sec * 1e3 + time.tv_nsec / 1e6;
}

int
main(int argc, char** argv)
{
	if (!X_setup())
		return EXIT_FAILURE;

	// event loop
	XEvent event;
	struct point last = { .x = -1, .y = -1 };
	while (!XNextEvent(dpy, &event)) {
		if (event.type == KeyPress) {
			KeySym key = XLookupKeysym(&event.xkey, 0);
			if (key == XK_Escape) {
				break;
			} else if (key == XK_Return) {
				break;
			}
		} else if (event.type == ButtonPress) {
			if (event.xbutton.button == Button3) {
				// Clear canvas when right-clicking
				XClearWindow(dpy, win);
			} else if (event.xbutton.button == Button1) {
				struct point p = { .x     = event.xbutton.x,
					               .y     = event.xbutton.y,
					               .msecs = get_msec() };

				if (p.x >= 0 && p.y >= 0) {
					XDrawPoint(dpy, win, gc, p.x, p.y);
					printf("x: %d, y: %d, t: %ld\n", p.x, p.y, p.msecs);
					last = p;
				}
			}
		} else if (event.type == ButtonRelease) {
			if (event.xbutton.button == Button1) {
				// EOL after releasing mouse click
				last.x = last.y = -1;
			}
		} else if (event.type == ClientMessage) {
			if (event.xclient.data.l[0] == wm_delete_msg) {
				break;
			}
		} else if (event.type == MotionNotify) {
			struct point p = { .x     = event.xmotion.x,
				               .y     = event.xmotion.y,
				               .msecs = get_msec() };

			// Draw continuous line
			if (last.x >= 0 && last.y >= 0)
				XDrawLine(dpy, win, gc, last.x, last.y, p.x, p.y);

			if (p.x >= 0 && p.y >= 0)
				printf("x: %d, y: %d, t: %ld\n", p.x, p.y, get_msec());

			last.x = p.x;
			last.y = p.y;
		}
	}

	X_destroy();
	return EXIT_SUCCESS;
}
