#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "../texify.h"
#include "detexify_kirelabs_org.h"

int
classify(char* data)
{
	CURL* hnd = curl_easy_init();

	curl_easy_setopt(hnd, CURLOPT_URL, CLASSIFIER_URL);
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, data);

	CURLcode ret = curl_easy_perform(hnd);

	curl_easy_cleanup(hnd);
	return (int)ret;
}

#define MAKE_FIT(_n)                                       \
	if (idx + (_n) > size) {                               \
		str = realloc(str, (size += (_n)) * sizeof(*str)); \
		if (!str) return NULL;                             \
	}

#define APPEND(_str)                       \
	do {                                   \
		size_t _size = strlen(_str);       \
		MAKE_FIT(_size);                   \
		strncpy(str + idx, (_str), _size); \
		idx += _size;                      \
	} while (0)

char*
encode(struct drawing* drawing)
{
	// minimum of 15 bytes (strokes=%5B%5B\0)
	char* str   = malloc(15 * sizeof(*str));
	size_t size = 15;
	size_t idx  = 0;

	APPEND("strokes=" OPEN_BRACKET);

	for (size_t i = 0; i < drawing->len; ++i) {
		if (i) APPEND(COMMA);

		APPEND(OPEN_BRACKET);

		struct shape* shape = &(drawing->shapes[i]);

		// Try to minimize realloc calls by approximating the needed size.
		// 63 was calculated by assuming that x & y are each 3 chars long and
		// the timestamp 13.
		MAKE_FIT(shape->len * 63 - (size - idx));

		for (size_t j = 0; j < shape->len; ++j) {
			if (j) APPEND(COMMA);

			struct point* p = &(shape->ps[j]);

			// The printed string should by far not exceed 100 characters
			MAKE_FIT(100 - (size - idx));

			// Add (url-encoded) '{"x":<x>,"y":<y>,"t":<time>}'
			idx += snprintf(str + idx, size - idx, "%s%d%s%d%s%ld%s",
			                OPEN_BRACE QUOTE "x" QUOTE COLON, p->x,
			                COMMA QUOTE "y" QUOTE COLON, p->y,
			                COMMA QUOTE "t" QUOTE COLON, p->msecs, CLOSE_BRACE);
		}

		APPEND(CLOSE_BRACKET);
	}

	// The APPEND macro is not able to append \0 properly as it uses strlen.
	// To make sure that enough space is allocated a simple 0 is temporarily
	// appended to then overwrite it.
	APPEND(CLOSE_BRACKET "0");
	str[idx - 1] = '\0';

	return str;
}
