#ifndef TEXIFY_H
#define TEXIFY_H

#include <stddef.h>

#define INIT_SHAPE_SIZE 100
#define INIT_DRAWING_SIZE 3

#define PADDING 0.1

// Simple point in 2d space with a timestamp when it was drawn
struct point {
	int x, y;
	long msecs;
};

// A shape holds all points that were drawn together between mouse press and
// release (while dragging).
// This can be just a single point, too.
struct shape {
	struct point* ps;
	size_t len, size;
};

// A drawing holds all shapes that were drawn onto the canvas.
struct drawing {
	struct shape* shapes;
	size_t len, size;

	int max_x, min_x;
	int max_y, min_y;
};

#endif /* TEXIFY_H */
