#ifndef CLASSIFIER_DETEXIFY_KIRELABS_ORG_H
#define CLASSIFIER_DETEXIFY_KIRELABS_ORG_H

#define CLASSIFIER_URL "http://detexify.kirelabs.org/api/classify"

#define COMMA "%2C"
#define COLON "%3A"
#define QUOTE "%22"
#define OPEN_BRACKET "%5B"
#define CLOSE_BRACKET "%5D"
#define OPEN_BRACE "%7B"
#define CLOSE_BRACE "%7D"

int classify(char* data);
char* encode(struct drawing* drawing);

#endif /* CLASSIFIER_DETEXIFY_KIRELABS_ORG_H */
