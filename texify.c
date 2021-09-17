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

struct drawing drawing;

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

void
print_drawing()
{
	printf("\n");
	for (size_t i = 0; i < drawing.len; ++i) {
		struct shape* shape = &(drawing.shapes[i]);
		for (size_t j = 0; j < shape->len; ++j) {
			struct point* p = &(shape->ps[j]);
			printf("x: %d, y: %d, msecs: %ld\n", p->x, p->y, p->msecs);
		}
		printf("\n");
	}
}

void
free_drawing()
{
	for (size_t i = 0; i < drawing.len; ++i) {
		free(drawing.shapes[i].ps);
	}
	free(drawing.shapes);
}

long
get_msec()
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return time.tv_sec * 1e3 + time.tv_nsec / 1e6;
}

int
add_point(struct point* p)
{
	if (!drawing.len)
		return 0;

	struct shape* shape = &drawing.shapes[drawing.len - 1];

	if (shape->len >= shape->size) {
		shape->ps = realloc(shape->ps, (shape->size += INIT_NUM_POINTS) *
		                                       sizeof(*shape->ps));
		if (!shape->ps)
			return 0;
	}

	shape->ps[shape->len] = *p;
	++shape->len;

	return 1;
}

int
create_shape()
{
	if (drawing.len >= drawing.size) {
		drawing.shapes = realloc(drawing.shapes,
		                         ++drawing.size * sizeof(*drawing.shapes));
		if (!drawing.shapes)
			return 0;
	}

	struct shape* shape = &drawing.shapes[drawing.len];

	shape->len  = 0;
	shape->size = INIT_NUM_POINTS;
	shape->ps   = malloc(shape->size * sizeof(*shape->ps));
	if (!shape->ps)
		return 0;

	++drawing.len;
	return 1;
}

int
main(int argc, char** argv)
{
	if (!X_setup())
		return EXIT_FAILURE;

	// Init drawing
	drawing.len    = 0;
	drawing.size   = INIT_DRAWING_SIZE;
	drawing.shapes = calloc(drawing.size, sizeof(*drawing.shapes));
	if (!drawing.shapes) {
		X_destroy();
		return EXIT_FAILURE;
	}

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
				create_shape();

				struct point p = { .x     = event.xbutton.x,
					               .y     = event.xbutton.y,
					               .msecs = get_msec() };

				if (p.x >= 0 && p.y >= 0) {
					XDrawPoint(dpy, win, gc, p.x, p.y);
					add_point(&p);
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
				add_point(&p);

			last.x = p.x;
			last.y = p.y;
		}
	}

	print_drawing();

	free_drawing();
	X_destroy();
	return EXIT_SUCCESS;
}
