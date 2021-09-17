#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>      // XLookupString(), XK_Escape (X11/keysymdef.h)
#include <X11/cursorfont.h> // XC_pencil

#define MIN_SIZE 1024

#define BRACKET_OPEN "%5B"
#define BRACKET_CLOSE "%5D"
#define COMMA "%2C"

int
main(int argc, char** argv)
{
	Display* dpy = XOpenDisplay(NULL);
	if (!dpy)
		return EXIT_FAILURE;
	int screen  = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	unsigned long black = BlackPixel(dpy, screen);
	unsigned long white = WhitePixel(dpy, screen);

	Window win;
	win = XCreateSimpleWindow(dpy, root, 100, 100, 100, 100, 10, white, black);

	// Listen to key and button presses and cursor dragging (move while click)
	XSelectInput(dpy, win,
	             KeyPressMask | ButtonPressMask | ButtonReleaseMask |
	                     Button1MotionMask);
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

	// Init POST data buffer
	size_t buf_size = MIN_SIZE;
	char* buf       = malloc(buf_size * sizeof(*buf));
	if (!buf)
		return EXIT_FAILURE; // TODO: Close X
	char* buf_start = buf;
	strncpy(buf, "strokes=%5B", 11);
	buf += 11;

	// event loop
	XEvent event;
	int last_X = -1, last_Y = -1;
	while (!XNextEvent(dpy, &event)) {
		if (event.type == KeyPress) {
			KeySym sym = XLookupKeysym(&event.xkey, 0);
			if (sym == XK_Escape) {
				*buf_start = 0;
				break;
			} else if (sym == XK_Return) {
				// remove trailing comma
				buf -= 3;

				if (buf_start + buf_size < buf + 10) {
					size_t buf_idx = buf - buf_start;
					buf_start      = realloc(buf_start, (buf_size += MIN_SIZE));
					buf            = buf_start + buf_idx;
				}
				strncpy(buf, BRACKET_CLOSE, 3);
				buf += 3;
				*buf = 0;
				break;
			}
		} else if (event.type == ButtonPress) {
			if (event.xbutton.button == Button3) {
				// Clear canvas when right-clicking
				XClearWindow(dpy, win);
				// Reset buffer (Keep "strokes=%5B")
				buf = buf_start + 11;
			} else if (event.xbutton.button == Button1) {
				if (buf_start + buf_size <= buf + 3) {
					size_t buf_idx = buf - buf_start;
					buf_start      = realloc(buf_start, (buf_size += MIN_SIZE));
					buf            = buf_start + buf_idx;
				}
				strncpy(buf, BRACKET_OPEN, 3);
				buf += 3;
			}
		} else if (event.type == ButtonRelease) {
			if (event.xbutton.button == Button1) {
				// EOL after releasing mouse click
				last_X = last_Y = -1;

				// remove trailing comma
				buf -= 3;

				if (buf_start + buf_size <= buf + 9) {
					size_t buf_idx = buf - buf_start;
					buf_start      = realloc(buf_start, (buf_size += MIN_SIZE));
					buf            = buf_start + buf_idx;
				}
				strncpy(buf, BRACKET_CLOSE, 3);
				buf += 3;
				strncpy(buf, COMMA, 3);
				buf += 3;
			}
		} else if (event.type == ClientMessage) {
			if (event.xclient.data.l[0] == wm_delete_msg) {
				*buf_start = 0;
				break;
			}
		} else if (event.type == MotionNotify) {
			XMotionEvent xmotion = event.xmotion;

			// Draw continuous line
			if (last_X >= 0 && last_Y >= 0)
				XDrawLine(dpy, win, gc, last_X, last_Y, xmotion.x, xmotion.y);

			if (xmotion.x >= 0 && xmotion.y >= 0) {
				if (buf_start + buf_size <= buf + 100) {
					size_t buf_idx = buf - buf_start;
					buf_start      = realloc(buf_start, (buf_size += MIN_SIZE));
					buf            = buf_start + buf_idx;
				}

				// Get milliseconds
				long t_msec;
				struct timespec time;
				clock_gettime(CLOCK_REALTIME, &time);
				t_msec = time.tv_sec * 1e3 + time.tv_nsec / 1e6;

				// Add '{"x":<x>,"y":<y>,"t":<time>},'
				buf += snprintf(
						buf, buf_size - (buf - buf_start),
						"%%7B%%22x%%22%%3A%d%%2C%%22y%%22%%3A%d%%2C%%22t%%22%%3A%ld%%7D%%2C",
						xmotion.x, xmotion.y, t_msec);
				printf("x: %d, y: %d, t: %ld\n", xmotion.x, xmotion.y, t_msec);
			}

			last_X = xmotion.x;
			last_Y = xmotion.y;
		}
	}

	*buf = 0;
	printf("%s\n", buf_start);
	free(buf_start);

	XFreeCursor(dpy, pencil_cursor);
	XFreeGC(dpy, gc);
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
