#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>      // XLookupString(), XK_Escape (X11/keysymdef.h)
#include <X11/cursorfont.h> // XC_pencil

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
	int last_X = -1, last_Y = -1;
	while (!XNextEvent(dpy, &event)) {
		if (event.type == KeyPress) {
			if (XLookupKeysym(&event.xkey, 0) == XK_Escape) {
				break;
			}
		} else if (event.type == ButtonPress) {
			if (event.xbutton.button == Button3) {
				// Clear canvas when right-clicking
				XClearWindow(dpy, win);
			} else if (event.xbutton.button == Button1) {
				XButtonPressedEvent xbutton = event.xbutton;

				if (xbutton.x >= 0 && xbutton.y >= 0) {
					// Draw dot
					XDrawPoint(dpy, win, gc, xbutton.x, xbutton.y);

					printf("x: %d, y: %d, t: %ld\n", xbutton.x, xbutton.y,
					       get_msec());

					last_X = xbutton.x;
					last_Y = xbutton.y;
				}
			}
		} else if (event.type == ButtonRelease) {
			if (event.xbutton.button == Button1) {
				// EOL after releasing mouse click
				last_X = last_Y = -1;
			}
		} else if (event.type == ClientMessage) {
			if (event.xclient.data.l[0] == wm_delete_msg) {
				break;
			}
		} else if (event.type == MotionNotify) {
			XMotionEvent xmotion = event.xmotion;

			// Draw continuous line
			if (last_X >= 0 && last_Y >= 0)
				XDrawLine(dpy, win, gc, last_X, last_Y, xmotion.x, xmotion.y);

			if (xmotion.x >= 0 && xmotion.y >= 0)
				printf("x: %d, y: %d, t: %ld\n", xmotion.x, xmotion.y,
				       get_msec());

			last_X = xmotion.x;
			last_Y = xmotion.y;
		}
	}

	X_destroy();
	return EXIT_SUCCESS;
}
