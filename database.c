#include <database.h>

ERROR *createError() {
	error *err = (error *) malloc(sizeof(error));
	return err;
}

void setError(ERROR *err, int code, char *msg) {
	err->code = code;
	err->message = msg;
}

void destroyError(ERROR *err) {
	free(err);
}