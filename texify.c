#include <stdio.h>
#include <stdlib.h>

#include <X11/cursorfont.h> // XC_pencil
#include <X11/Xlib.h>
#include <X11/Xutil.h>      // XLookupString(), XK_Escape (X11/keysymdef.h)

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
	win = XCreateSimpleWindow(dpy, root, 100, 100, 100, 100, 10, white, black);

	// Listen to key and button presses and cursor dragging (move while click)
	XSelectInput(dpy, win, KeyPressMask | ButtonPressMask | Button1MotionMask);
	// Listen for WM_DELETE_WINDOW message
	Atom wm_delete_msg = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(dpy, win, &wm_delete_msg, 1);

	// Display the window
	XMapWindow(dpy, win);

	// Use pencil as cursor
	Cursor pencil_cursor = XCreateFontCursor(dpy, XC_pencil);
	XDefineCursor(dpy, win, pencil_cursor);

	// Sync
	XSync(dpy, False);

	// Setup drawing
	GC gc = XCreateGC(dpy, win, 0, NULL);
	XSetForeground(dpy, gc, white);

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
				break;
			}
		} else if (event.type == ClientMessage) {
			if (event.xclient.data.l[0] == wm_delete_msg) {
				break;
			}
		} else if (event.type == MotionNotify) {
			XMotionEvent xmotion = event.xmotion;
			printf("x: %d, y: %d\n", xmotion.x, xmotion.y);

			if (last_X >= 0 && last_Y >= 0)
				XDrawLine(dpy, win, gc, last_X, last_Y, xmotion.x, xmotion.y);

			last_X = xmotion.x;
			last_Y = xmotion.y;
		}
	}

	XFreeCursor(dpy, pencil_cursor);
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
